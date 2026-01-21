#!/usr/bin/env python3
from __future__ import annotations

import math
from pathlib import Path

import pandas as pd

CRITERIA_BASE_NAMES = {
    "c10": "Criterion 10",
    "c11": "Criterion 11",
    "c12": "Criterion 12",
    "c13": "Criterion 13",
    "c30": "Criterion 30",
    "c51": "Criterion 51",
}

TEXT_ID_NAMES = {
    "plain": "Plain text",
    "vig1": "Vigenère (key length 1)",
    "vig5": "Vigenère (key length 5)",
    "vig10": "Vigenère (key length 10)",
    "affine_sym": "Affine cipher (symbolic)",
    "affine_bi": "Affine cipher (bigrams)",
}

# =======================
# HARD-CODED PATHS
# =======================
CSV_FILES = [
    Path("./criteria_stats.csv"),
]

OUTPUT_DIR = Path("./report/")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


# =======================
# HELPERS
# =======================
def latex_escape(s: str) -> str:
    return (
        s.replace("\\", r"\textbackslash{}")
        .replace("&", r"\&")
        .replace("%", r"\%")
        .replace("$", r"\$")
        .replace("#", r"\#")
        .replace("_", r"\_")
        .replace("{", r"\{")
        .replace("}", r"\}")
    )


def is_nonzero(x) -> bool:
    try:
        v = float(x)
        return not math.isclose(v, 0.0)
    except Exception:
        return False


def fmt_float(x, digits: int = 6) -> str:
    try:
        v = float(x)
        return f"{v:.{digits}f}".rstrip("0").rstrip(".")
    except Exception:
        return "-"


# =======================
# LATEX RENDERING
# =======================
def build_params_math(prefix: str, threshold_val, j_val) -> list[str]:
    parts = []
    if is_nonzero(threshold_val):
        parts.append(rf"\theta_{{{prefix}}}=" + fmt_float(threshold_val))
    if is_nonzero(j_val):
        parts.append(rf"j_{{{prefix}}}=" + str(int(j_val)))
    return parts


def build_name_with_params(crit_base: str, sy: dict | None, bi: dict | None) -> str:
    base_name = CRITERIA_BASE_NAMES.get(crit_base, crit_base)
    base_tex = latex_escape(base_name)

    params = []
    if sy is not None:
        params.extend(build_params_math("sy", sy.get("threshold", 0), sy.get("j", 0)))
    if bi is not None:
        params.extend(build_params_math("bi", bi.get("threshold", 0), bi.get("j", 0)))

    if not params:
        return base_tex

    # thresholds/j are rendered right next to the name, in math mode
    return base_tex + r" ($" + ", ".join(params) + r"$)"


def split_criteria_id(criteria_id: str) -> tuple[str, str]:
    # "c30sy" -> ("c30", "sy"), "c51bi" -> ("c51", "bi")
    if len(criteria_id) < 4:
        return criteria_id, ""
    base = criteria_id[:3]  # "c30"
    kind = criteria_id[3:]  # "sy" / "bi"
    return base, kind


def build_param_tex(threshold_val, j_val) -> str:
    params = []
    if is_nonzero(threshold_val):
        params.append(r"\theta=" + fmt_float(threshold_val))
    if is_nonzero(j_val):
        params.append(r"j=" + str(int(j_val)))
    if not params:
        return ""
    return r" ($" + ", ".join(params) + r"$)"


def render_table_for_text_id(df: pd.DataFrame, text_id: str) -> str:
    # Expect both sy/bi for same text_id, but tolerate missing.
    # Build rows indexed by base criterion ("c10", "c11", ...)
    rows: dict[str, dict[str, dict[str, object]]] = {}

    for _, r in df.iterrows():
        crit_base, kind = split_criteria_id(str(r["criteria_id"]))
        if crit_base not in rows:
            rows[crit_base] = {}
        rows[crit_base][kind] = {
            "fp": r.get("fp", float("nan")),
            "fn": r.get("fn", float("nan")),
            "threshold": r.get("threshold", 0),
            "j": r.get("j", 0),
        }

    # Deterministic order by numeric part: c10, c11, c12, c13, c30, c51...
    def crit_sort_key(c: str) -> tuple[int, str]:
        try:
            return (int(c[1:]), c)
        except Exception:
            return (10**9, c)

    ordered_bases = sorted(rows.keys(), key=crit_sort_key)
    # L is constant inside one table – take the first value
    L_val = int(df["l"].iloc[0])
    text_title = TEXT_ID_NAMES.get(str(text_id), str(text_id))

    lines = []
    lines.append(r"\begin{center}")
    lines.append(r"\renewcommand{\arraystretch}{1.2}")
    lines.append(r"\setlength{\tabcolsep}{6pt}")
    lines.append(r"\begin{tabular}{|l||c|c||c|c|}")
    lines.append(r"\hline")
    lines.append(
        r"\multicolumn{5}{|c|}{\textbf{Text: "
        + latex_escape(text_title)
        + f", $L={L_val}$"
        + r"}} \\"
    )
    lines.append(r"\hline")
    lines.append(
        r"\textbf{Criteria} & \multicolumn{2}{c||}{\textbf{Symbolic}} "
        r"& \multicolumn{2}{c|}{\textbf{Bigram}} \\"
    )
    lines.append(r"\hline")
    lines.append(r" & \textbf{FP} & \textbf{FN} & \textbf{FP} & \textbf{FN} \\")
    lines.append(r"\hline")

    for crit_base in ordered_bases:
        sy = rows[crit_base].get("sy")
        bi = rows[crit_base].get("bi")

        crit_cell = build_name_with_params(crit_base, sy, bi)

        sy_fp = fmt_float(sy["fp"]) if sy is not None else "-"
        sy_fn = fmt_float(sy["fn"]) if sy is not None else "-"
        bi_fp = fmt_float(bi["fp"]) if bi is not None else "-"
        bi_fn = fmt_float(bi["fn"]) if bi is not None else "-"

        lines.append(
            crit_cell
            + " & "
            + sy_fp
            + " & "
            + sy_fn
            + " & "
            + bi_fp
            + " & "
            + bi_fn
            + r" \\"
        )

    lines.append(r"\hline")
    lines.append(r"\end{tabular}")
    lines.append(r"\end{center}")
    lines.append("")

    return "\n".join(lines)


# =======================
# MAIN
# =======================
def main() -> None:
    for csv_path in CSV_FILES:
        df = pd.read_csv(csv_path)

        out_path = OUTPUT_DIR / f"{csv_path.stem}_tables.tex"

        chunks = []
        for (text_id, L_val), grp in df.groupby(["text_id", "l"], sort=False):
            chunks.append(render_table_for_text_id(grp, str(text_id)))

        out_path.write_text("\n".join(chunks), encoding="utf-8")

        print(f"[OK] Written: {out_path}")


if __name__ == "__main__":
    main()
