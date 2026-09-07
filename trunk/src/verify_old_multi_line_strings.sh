#!/bin/bash
#
# verify_old_multi_line_strings.sh -- regression test for
# Bugs28 #100(i): Tokenizer::tokenize_string1() (the '...' string
# tokenizer) unconditionally accepted an unterminated string in a
# function body (⎕FX, the ∇ editor), silently treating it as closed at
# end-of-line -- regardless of the OLD-MULTI-LINE-STRINGS preference,
# which the sibling "..."/«...» string tokenizer already honoured.
# A user who explicitly disables that preference (to get strict,
# typo-catching behaviour) still silently got the leniency for '...'
# strings, the far more commonly used quoting style.
#
# The preference is read once from $HOME/.gnu-apl/preferences at
# startup, so this needs its own interpreter invocation with a
# dedicated HOME -- an ordinary .tc file (a single already-started
# interpreter, default preferences) cannot exercise the disabled case.
# See testcases/Quad_FX.tc for the (unaffected) default-preference
# regression.
#
# Usage: ./verify_old_multi_line_strings.sh   (from the src/ directory, after 'make')

set -u

if [[ ! -x ./apl ]]; then
   echo "verify_old_multi_line_strings.sh: must be run from the src/" \
        "directory (need ./apl here)" >&2
   exit 1
fi

fail=0

# run_apl SCRIPT_TEXT OLD_MULTI_LINE_STRINGS
#
# runs apl with a fresh HOME whose preferences file sets
# OLD-MULTI-LINE-STRINGS to the given value ("Yes"/"No"), feeds it the
# given APL text followed by ')OFF', and prints stdout+stderr.
run_apl()
{
   local script=$1 setting=$2
   local run_home
   run_home=$(mktemp -d)
   mkdir -p "$run_home/.gnu-apl"
   echo "OLD-MULTI-LINE-STRINGS $setting" > "$run_home/.gnu-apl/preferences"

   HOME="$run_home" ./apl --script <<< "$script"$'\n)OFF' 2>&1
   rm -rf "$run_home"
}

echo "== OLD-MULTI-LINE-STRINGS Yes (default): '...' leniency unaffected =="
out=$(run_apl "⎕FX 'F' 'A←''abc' 'B←5'
F
A" Yes)
if [[ $out == *"abc"* ]]; then
   echo "PASS: leniency still accepts the unterminated string"
else
   echo "FAIL: expected the string to still be silently accepted, got:"
   echo "$out"
   fail=1
fi

echo
echo "== OLD-MULTI-LINE-STRINGS No: '...' now rejects an unterminated string =="
# ⎕FX catches the parse failure itself and returns the 1-based error
# line number instead of the function name -- ⎕NC 'F' staying 0
# (never defined) is what actually distinguishes "rejected" from
# "silently accepted" here.
out=$(run_apl "⎕FX 'F' 'A←''abc' 'B←5'
⎕NC 'F'" No)
if [[ $out == *$'\n'"0" || $out == "0" ]]; then
   echo "PASS: F was never defined -- unterminated '...' string rejected"
else
   echo "FAIL: expected ⎕NC 'F' to be 0 (undefined), got:"
   echo "$out"
   fail=1
fi

echo
if (( fail )); then
   echo "verify_old_multi_line_strings.sh: FAILED"
   exit 1
fi
echo "verify_old_multi_line_strings.sh: all checks passed"
