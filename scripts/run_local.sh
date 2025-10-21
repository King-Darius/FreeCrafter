#!/usr/bin/env bash
set -euo pipefail
export STREAMLIT_SERVER_SHOW_EMAIL_PROMPT=false
export STREAMLIT_BROWSER_GATHER_USAGE_STATS=false
python -m streamlit run lofi_symphony/app.py
