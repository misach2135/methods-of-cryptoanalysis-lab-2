#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations

import argparse
import re
import sys
from io import StringIO
from pathlib import Path
from typing import Optional

import pandas as pd


CRIT_RE = re.compile(r"c(\d+)")


def fmt_num(v: object, *, decimals: int = 6) -> str:
    """
    Format numbers for LaTeX tables:
    - keep floats (no scientific notation for typical lab values)
    - drop trailing zeros
    - show integers as '1', '0'
    """
    if v is None:
        return "0"

    try:
        x = float(v)
    except (TypeError, ValueError):
        return latex_escape(str(v))

    # Avoid "-0"
    if abs(x) < 0.5 * 10 ** (-decimals):
        x = 0.0

    # If it's effectively an integer, print as int
    if abs(x - round(x)) < 1e-12:
        return str(int(round(x)))

    s = f"{x:.{decimals}f}".rstrip("0").rstrip(".")
    return s if s else "0"


def table_title_from_text_id(text_id: str) -> str:
    """
    Map internal text_id to human-readable LaTeX table title.
    """

    mapping = {
        # plain
        "plain": "Вихідний (неспотворений) текст",
        # vigenere
        "vig1": "Спотворення за допомогою шифру Віженера ($r = 1$)",
        "vig5": "Спотворення за допомогою шифру Віженера ($r = 5$)",
        "vig10": "Спотворення за допомогою шифру Віженера ($r = 10$)",
        # affine
        "affine_sym": "Спотворення за допомогою афінного шифру (1-грамний)",
        "affine_bi": "Спотворення за допомогою афінного шифру (2-грамний)",
        # random
        "random": "Випадковий текст",
        # compressed plain
        "c_plain": "Стиснений вихідний текст",
        # compressed vigenere
        "c_vig1": "Стиснений текст після шифру Віженера ($r = 1$)",
        "c_vig5": "Стиснений текст після шифру Віженера ($r = 5$)",
        "c_vig10": "Стиснений текст після шифру Віженера ($r = 10$)",
        # compressed affine
        "c_affine_sym": "Стиснений текст після афінного шифру (1-грамний)",
        "c_affine_bi": "Стиснений текст після афінного шифру (2-грамний)",
        # compressed random
        "c_random": "Стиснений випадковий текст",
    }

    try:
        return mapping[text_id]
    except KeyError:
        return f"Невідомий тип тексту: \\texttt{{{text_id}}}"


def parse_criteria_num(criteria_id: str) -> int:
    m = CRIT_RE.search(str(criteria_id))
    if not m:
        raise ValueError(
            f"Can't parse criteria number from criteria_id={criteria_id!r}"
        )
    return int(m.group(1))


def read_input_csv(path: Optional[str]) -> pd.DataFrame:
    if path is None or path == "-":
        raw = sys.stdin.read()
        if not raw.strip():
            raise ValueError("No input provided on stdin.")
        return pd.read_csv(StringIO(raw))
    return pd.read_csv(path)


def build_pivot(df: pd.DataFrame) -> pd.DataFrame:
    required = ["l", "lgramSize", "criteria_id", "text_id", "fp", "fn"]
    missing = [c for c in required if c not in df.columns]
    if missing:
        raise ValueError(f"Missing required columns: {missing}")

    work = df.copy()
    work["l"] = work["l"].astype(int)
    work["lgramSize"] = work["lgramSize"].astype(int)
    work["criteria_num"] = work["criteria_id"].map(parse_criteria_num)

    # IMPORTANT: keep fp/fn as float
    work["fp"] = pd.to_numeric(work["fp"], errors="raise").astype(float)
    work["fn"] = pd.to_numeric(work["fn"], errors="raise").astype(float)

    agg = work.groupby(["text_id", "l", "criteria_num", "lgramSize"], as_index=False)[
        ["fp", "fn"]
    ].sum()

    pv = agg.pivot_table(
        index=["text_id", "l", "criteria_num"],
        columns="lgramSize",
        values=["fp", "fn"],
        aggfunc="sum",
        fill_value=0.0,  # float
    )

    pv.columns = [f"{metric}_{gram}" for (metric, gram) in pv.columns.to_flat_index()]
    pv = pv.reset_index()

    for col in ("fp_1", "fn_1", "fp_2", "fn_2"):
        if col not in pv.columns:
            pv[col] = 0.0

    pv = pv[["text_id", "l", "criteria_num", "fp_1", "fn_1", "fp_2", "fn_2"]].copy()
    pv = pv.sort_values(["text_id", "l", "criteria_num"], kind="mergesort").reset_index(
        drop=True
    )

    # Ensure float dtype
    for c in ["fp_1", "fn_1", "fp_2", "fn_2"]:
        pv[c] = pd.to_numeric(pv[c], errors="coerce").fillna(0.0).astype(float)

    return pv


def latex_escape(s: str) -> str:
    # Мінімально потрібно для твоїх назв
    return (
        s.replace("\\", r"\textbackslash{}")
        .replace("&", r"\&")
        .replace("%", r"\%")
        .replace("_", r"\_")
        .replace("#", r"\#")
        .replace("{", r"\{")
        .replace("}", r"\}")
        .replace("~", r"\textasciitilde{}")
        .replace("^", r"\textasciicircum{}")
    )


def render_rows_with_multirow_by_L(pv_text: pd.DataFrame) -> list[str]:
    lines: list[str] = []

    pv_text = pv_text.sort_values(["l", "criteria_num"], kind="mergesort").reset_index(
        drop=True
    )

    for L, grpL in pv_text.groupby("l", sort=True):
        grpL = grpL.sort_values(["criteria_num"], kind="mergesort").reset_index(
            drop=True
        )
        n = len(grpL)
        if n == 0:
            continue

        for i, row in enumerate(grpL.itertuples(index=False)):
            crit = int(row.criteria_num)
            fp1 = fmt_num(row.fp_1)
            fn1 = fmt_num(row.fn_1)
            fp2 = fmt_num(row.fp_2)
            fn2 = fmt_num(row.fn_2)

            if i == 0:
                lines.append(
                    rf"\multirow{{{n}}}{{*}}{{{int(L)}}} & {crit} & {fp1} & {fn1} & {fp2} & {fn2} \\"
                )
            else:
                lines.append(rf" & {crit} & {fp1} & {fn1} & {fp2} & {fn2} \\")
        lines.append(r"\hline")  # only once per L-block

    return lines


def render_table_for_text(pv_text: pd.DataFrame, title: str) -> str:
    lines: list[str] = []
    lines.append(r"\begin{center}")
    lines.append(r"\renewcommand{\arraystretch}{1.2}")
    lines.append(r"\setlength{\tabcolsep}{6pt}")
    lines.append(r"\begin{tabular}{|c|l||c|c||c|c|}")
    lines.append(r"\hline")
    lines.append(r"\multicolumn{6}{|c|}{" + latex_escape(title) + r"} \\")
    lines.append(r"\hline")
    lines.append(
        r"\textit{L} & \textit{Номер критерію} & "
        r"\textit{FP} ($l=1$) & \textit{FN} ($l=1$) & "
        r"\textit{FP} ($l=2$) & \textit{FN} ($l=2$) \\"
    )
    lines.append(r"\hline")

    lines.extend(render_rows_with_multirow_by_L(pv_text))

    lines.append(r"\end{tabular}")
    lines.append(r"\end{center}")
    lines.append("")
    return "\n".join(lines)


def render_table_for_L(grp: pd.DataFrame, title: str) -> str:
    grp = grp.sort_values(["criteria_num"], kind="mergesort").reset_index(drop=True)
    n = len(grp)

    lines: list[str] = []
    lines.append(r"\begin{center}")
    lines.append(r"\renewcommand{\arraystretch}{1.2}")
    lines.append(r"\setlength{\tabcolsep}{6pt}")
    lines.append(r"\begin{tabular}{|c|l||c|c||c|c|}")
    lines.append(r"\hline")
    lines.append(r"\multicolumn{6}{|c|}{" + latex_escape(title) + r"} \\")
    lines.append(r"\hline")
    lines.append(
        r"\textit{L} & \textit{Номер критерію} & "
        r"\textit{FP} ($l=1$) & \textit{FN} ($l=1$) & "
        r"\textit{FP} ($l=2$) & \textit{FN} ($l=2$) \\"
    )
    lines.append(r"\hline")

    if n == 0:
        lines.append(r"\end{tabular}")
        lines.append(r"\end{center}")
        lines.append("")
        return "\n".join(lines)

    L0 = int(grp.iloc[0].l)

    for i, row in enumerate(grp.itertuples(index=False)):
        crit = int(row.criteria_num)
        fp1 = fmt_num(row.fp_1)
        fn1 = fmt_num(row.fn_1)
        fp2 = fmt_num(row.fp_2)
        fn2 = fmt_num(row.fn_2)

        if i == 0:
            lines.append(
                rf"\multirow{{{n}}}{{*}}{{{L0}}} & {crit} & {fp1} & {fn1} & {fp2} & {fn2} \\"
            )
        else:
            lines.append(rf" & {crit} & {fp1} & {fn1} & {fp2} & {fn2} \\")

    lines.append(r"\hline")
    lines.append(r"\end{tabular}")
    lines.append(r"\end{center}")
    lines.append("")
    return "\n".join(lines)


def to_latex_document(pv: pd.DataFrame) -> str:
    parts = []
    parts.append(r"\documentclass[a4paper,12pt]{article}")
    parts.append(r"\usepackage[utf8]{inputenc}")
    parts.append(r"\usepackage[T2A]{fontenc}")
    parts.append(r"\usepackage[ukrainian]{babel}")
    parts.append(r"\usepackage{multirow}")
    parts.append(r"\usepackage[margin=1in]{geometry}")
    parts.append(r"\begin{document}")
    parts.append("")

    for text_id, pv_text in pv.groupby("text_id", sort=True):
        title_prefix = table_title_from_text_id(text_id)

        for L, grp in pv_text.groupby("l", sort=True):
            title = f"{title_prefix} (L = {int(L)})"
            parts.append(render_table_for_L(grp, title))

    parts.append(r"\end{document}")
    parts.append("")
    return "\n".join(parts)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Sort criteria results by L, lgramSize and criteria; export LaTeX tables."
    )
    ap.add_argument(
        "--in", dest="in_path", default="-", help="Input CSV path or '-' for stdin"
    )
    ap.add_argument(
        "--out", dest="out_path", default="report.tex", help="Output .tex path"
    )
    ap.add_argument(
        "--title",
        dest="title",
        default="Results",
        help="Table title prefix",
    )
    args = ap.parse_args()

    df = read_input_csv(args.in_path)
    pv = build_pivot(df)
    tex = to_latex_document(pv)

    Path(args.out_path).write_text(tex, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
