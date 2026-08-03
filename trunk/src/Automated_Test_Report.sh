#!/bin/bash
#
# Automated_Test_Report.sh -- narrow down a testcase failure to a minimal
# reproducing set, and package the findings into a tarball ready to email.
#
# When a testcase fails only in the context of other testcases that ran
# before it (shared-workspace interference, not a bug in the failing file
# itself), this script finds a minimal ordered subset of the earlier
# testcases that still reproduces the same failure -- so the interference
# can be root-caused without re-running the whole suite by hand. Its own
# narration is written to culprits.log next to summary.log, and its
# findings (culprits.log plus the failing testcase's own .tc.log) are
# archived into culprits.tar.gz for emailing to bug-apl@gnu.org.
#
# Usage: (from the src/ directory, after 'make test' or 'make test3' has
# produced a summary.log with a failure in it)
#   ./Automated_Test_Report.sh [logdir] [selection]
#
# logdir defaults to "testcases" and must contain a summary.log (as written
# by e.g. 'make test' -> testcases/summary.log, or 'make test3' ->
# testcases_3/summary.log).
#
# selection is only used when summary.log has more than one failing
# testcase (see step 2b below): a 1-based number picking which one to
# isolate, in the numbered list this script would otherwise prompt for
# interactively -- pass it to run non-interactively (e.g. from a cron job
# or CI) instead of being asked.
#
# Algorithm (see plan.txt):
#   1. Locate summary.log.
#   2. Parse it for the ordered list of testcases that were run, each
#      one's error count, and every one with a nonzero error count -- this
#      list is kept as an in-memory array throughout, never re-derived
#      from a shell wildcard (globs get re-sorted by the shell, which is
#      exactly the ordering trap this script exists to avoid).
#   2b. If more than one testcase failed, show them as a numbered list and
#      ask which one to isolate (or take it from the command line) --
#      a single run's summary.log cannot say by itself which of several
#      failures is an independent bug and which are follow-on damage from
#      an earlier one. With only one failure, nothing to choose.
#   3. Drop every testcase that ran after the chosen failure -- irrelevant.
#   3b. First reduction pass: if any OTHER testcase that ran before the
#      chosen one also failed, try dropping all of them at once first, to
#      directly answer the question Step 2b's dialog raises (independent
#      bug, or follow-on effect of another already-known failure?) before
#      the general-purpose minimization below has to work it out the hard
#      way.
#   4. Re-run apl once to confirm the resulting set still reproduces the
#      same failure (the log may be stale).
#   5. Bisect: repeatedly try halving the remaining candidate set.
#   6. Remove candidates one at a time until every remaining one is
#      individually required to reproduce the same failure.
#
# Every step below is narrated as Step / Action / Boundary Condition /
# Consequence -- see plan.txt for the intent: a reader of culprits.log
# should be able to reconstruct what was done, why, what the result of
# each action was, and what was concluded from it, without having
# watched the run live.

set -u

# ---------------------------------------------------------------------------
# box-drawing helpers for the Step / Action / Boundary Condition /
# Consequence narration. Boxes are word-wrapped to fit a 79-char terminal
# and start at column 0 (no leading indentation), to leave as much room
# as possible for the actual text.
# ---------------------------------------------------------------------------
MAX_WIDTH=79

rep() { local c=$1 n=$2 s="" i; for ((i = 0; i < n; i++)); do s+=$c; done; printf '%s' "$s"; }

# wrap_text TEXT MAXLEN -- word-wrap TEXT into lines of at most MAXLEN
# characters (never splits a single word, even one longer than MAXLEN),
# leaving the result in the WRAP_LINES array.
wrap_text()
{
   local text=$1 maxlen=$2
   WRAP_LINES=()
   local line="" word
   for word in $text; do
      if [[ -z "$line" ]]; then
         line=$word
      elif (( ${#line} + 1 + ${#word} <= maxlen )); then
         line+=" $word"
      else
         WRAP_LINES+=("$line")
         line=$word
      fi
   done
   WRAP_LINES+=("$line")
}

box()   # box BORDER_CHAR CORNER_TL CORNER_TR CORNER_BL CORNER_BR SIDE LABEL TEXT
{
   local hc=$1 tl=$2 tr=$3 bl=$4 br=$5 side=$6 label=$7 text=$8

   # interior width budget: MAX_WIDTH total, minus the 2 side/corner chars;
   # each content line is rendered as " <wrapped line> " (1 space padding
   # either side), so word-wrap to 2 less than that.
   local interior=$(( MAX_WIDTH - 2 ))
   local maxlen=$(( interior - 2 ))
   (( maxlen < 10 )) && maxlen=10   # sane floor for a tiny MAX_WIDTH

   wrap_text "$text" "$maxlen"

   local width=${#label}
   local wl w
   for wl in "${WRAP_LINES[@]}"; do
      w=$(( ${#wl} + 2 ))
      (( w > width )) && width=$w
   done
   # width can still exceed interior if a single word (e.g. a long path)
   # couldn't be wrapped -- left as-is rather than truncating data.

   local lead tail
   if (( width == ${#label} )); then
      lead=0; tail=0
   else
      lead=4
      tail=$(( width - lead - ${#label} ))
      (( tail < 0 )) && { lead=0; tail=$(( width - ${#label} )); }
   fi

   echo
   printf '%s%s%s%s%s\n' "$tl" "$(rep "$hc" $lead)" "$label" "$(rep "$hc" $tail)" "$tr"
   for wl in "${WRAP_LINES[@]}"; do
      local content=" $wl "
      while (( ${#content} < width )); do content+=' '; done
      printf '%s%s%s\n' "$side" "$content" "$side"
   done
   printf '%s%s%s\n' "$bl" "$(rep "$hc" $width)" "$br"
}

STEP=0

# step TITLE... -- start a new Step. Numbering only ever increases here
# (never resets) -- every Action/Boundary Condition/Consequence box
# below it, however many there are before the next Step, repeats this
# same number rather than counting up its own sub-index. That way a
# reader scanning top to bottom never sees the number appear to jump
# backward (e.g. Step #8's boxes ending at a sub-count, immediately
# followed by Step #9's boxes restarting at #1).
step()
{
   (( ++STEP ))
   box '═' '╔' '╗' '╚' '╝' '║' " Step #$STEP " "$*"
}

# action TEXT... -- start a new Action/Boundary Condition/Consequence unit
# within the current Step. There may be several such units per Step; all
# of them share the enclosing Step's number.
action()
{
   box '─' '┌' '┐' '└' '┘' '│' " Action #$STEP " "$*"
}

# boundary TEXT... -- the Boundary Condition for the current unit.
boundary()
{
   box '─' '┌' '┐' '└' '┘' '│' " Boundary Condition #$STEP " "$*"
}

# consequence TEXT... -- the Consequence drawn for the current unit.
consequence()
{
   box '─' '┌' '┐' '└' '┘' '│' " Consequence #$STEP " "$*"
}

# ---------------------------------------------------------------------------
# 1. must be run from src/
# ---------------------------------------------------------------------------
if [[ ! -x ./apl || ! -d testcases ]]; then
   echo "Automated_Test_Report.sh: must be run from the src/ directory" \
        "(need ./apl and testcases/ here)" >&2
   exit 1
fi

SRC_DIR=$(pwd)
WORK=$SRC_DIR/tmp/testcases
RUN_COUNT=0

# set by Step 4 if the log-derived candidate set does not reproduce right
# now (a flaky/non-deterministic failure, e.g. a crash inside a
# third-party library that only triggers some fraction of the time).
# Minimization (Step 5/6) requires a reliable reproduction signal and is
# meaningless without one, so degraded mode skips it and instead packages
# whatever original evidence the run that produced $SUMMARY already left
# behind on disk -- see Step 4, 8, 9, 10 below.
DEGRADED=no

step "locate summary.log"

action "check the command line for an explicit logdir argument"
if (( $# > 0 )); then
   boundary "an argument was given: '$1'"
   LOGDIR=$1
else
   boundary "no argument was given"
   LOGDIR=testcases
fi
consequence "logdir = '$LOGDIR'"

SUMMARY=$SRC_DIR/$LOGDIR/summary.log
CULPRITS_LOG=$SRC_DIR/$LOGDIR/culprits.log

# log every action (the same text shown on the terminal below) into
# culprits.log.new next to summary.log, in addition to printing it. Save
# the real stdout/stderr on fd 3/4 first so logging can be switched off
# again later, once culprits.log.new is complete (see step 8) -- at that
# point (and only then) it is renamed onto culprits.log, so a previous
# run's culprits.log is never deleted or left truncated, only atomically
# replaced once its successor is fully written.
exec 3>&1 4>&2
if [[ -d "$SRC_DIR/$LOGDIR" ]]; then
   exec > >(tee "$CULPRITS_LOG.new") 2>&1
   # clean up the scratch file on any exit that doesn't reach its rename
   # to culprits.log (e.g. aborting early below); replaced with a wider
   # trap further down once there is more to clean up on exit too -- this
   # one never touches the user-facing culprits.log itself.
   trap 'rm -f "$CULPRITS_LOG.new"' EXIT
fi

action "check whether '$SUMMARY' exists"
if [[ -f "$SUMMARY" ]]; then
   boundary "the file exists"
   consequence "using $SUMMARY, logging this run to $CULPRITS_LOG"
else
   boundary "the file does not exist"
   consequence "aborting -- run 'make test' (or 'make test3', with" \
               "logdir=testcases_3) first"
   echo "Automated_Test_Report.sh: '$SUMMARY' not found" >&2
   exit 1
fi

# apl -T unconditionally truncates the hardcoded path "testcases/summary.log"
# (relative to CWD) as soon as -T is parsed, regardless of which files -T
# actually names (UserPreferences.cc). Since every apl invocation below uses
# -T, that would otherwise silently wipe out the very log we're reading from
# (or clobber it even when LOGDIR is something else). Back it up now and
# restore it on exit, however this script ends. Also clean up
# culprits.log.new on any exit that doesn't reach its rename to
# culprits.log (e.g. this script aborting early) -- it is our own scratch
# file, never the user-facing culprits.log itself, which this trap never
# touches.
if [[ -f "$SRC_DIR/testcases/summary.log" ]]; then
   cp "$SRC_DIR/testcases/summary.log" "$SRC_DIR/testcases/summary.log.culprits_bak"
   trap 'mv -f "$SRC_DIR/testcases/summary.log.culprits_bak" "$SRC_DIR/testcases/summary.log" 2>/dev/null
         rm -f "$CULPRITS_LOG.new"' EXIT
else
   trap 'rm -f "$SRC_DIR/testcases/summary.log" "$CULPRITS_LOG.new"' EXIT
fi

# ---------------------------------------------------------------------------
# 2. parse summary.log: build the ordered testcase-name list, the matching
# original path (as recorded in the log, relative to $SRC_DIR) for each,
# each entry's error count, and the full list of entries with a nonzero
# error count (there may be more than one -- see Step 2b below).
# ---------------------------------------------------------------------------
step "parse summary.log for the run order and all failures"

action "read $SUMMARY line by line, extracting each entry's error count" \
       "and testcase path"

all_names=()
all_paths=()
all_errs=()
fail_idxs=()

# an entry with apl_errors > 0 (a testcase left something on the )SI stack,
# etc.) spans two lines: "N time (M APL,   loc=... .tc line=L" followed by
# "   X assert, Y diff, Z parse) path.tc" -- the leading error count N is
# on the FIRST line, but only the SECOND line ends in ".tc". Remember N
# across the two lines instead of (wrongly) reading the second line's own
# first field, which is X (assert_errors), not N.
pending_errs=""

while IFS= read -r line; do
   if [[ "$line" != *.tc ]]; then
      if [[ "$line" =~ ^[[:space:]]*([0-9]+)[[:space:]].*\(.*APL, ]]; then
         pending_errs=${BASH_REMATCH[1]}
      fi
      continue   # not (yet) an entry line -- header/separator/trailer/etc.
   fi

   local_path=${line##* }                    # last field: the file path
   if [[ -n "$pending_errs" ]]; then
      local_errs=$pending_errs
      pending_errs=""
   else
      read -r local_errs _ <<< "$line"        # first field: error count
   fi
   [[ "$local_errs" =~ ^[0-9]+$ ]] || continue

   all_paths+=("$local_path")
   all_names+=("$(basename "$local_path")")
   all_errs+=("$local_errs")

   if (( local_errs > 0 )); then
      fail_idxs+=($(( ${#all_names[@]} - 1 )))
   fi
done < "$SUMMARY"

if (( ${#all_names[@]} == 0 )); then
   boundary "no parseable testcase entries were found"
   consequence "aborting -- '$SUMMARY' does not look like a summary.log"
   echo "Automated_Test_Report.sh: could not parse any testcase entries out of" \
        "'$SUMMARY'" >&2
   exit 1
fi

if (( ${#fail_idxs[@]} == 0 )); then
   boundary "${#all_names[@]} entries parsed, none with a nonzero" \
            "error count"
   consequence "nothing to narrow down -- the recorded run passed cleanly"
   exit 0
fi

boundary "${#all_names[@]} entries parsed; ${#fail_idxs[@]} with a" \
         "nonzero error count"
if (( ${#fail_idxs[@]} == 1 )); then
   consequence "only one failing testcase -- nothing to choose between"
else
   consequence "more than one failing testcase -- which to isolate is" \
               "decided next (Step 2b)"
fi

# ---------------------------------------------------------------------------
# 2b. if more than one testcase failed, a single run's summary.log cannot
# tell, by itself, which of them is an independent bug and which are mere
# follow-on damage from an earlier one (shared-workspace interference is
# exactly what this script exists to untangle in the first place) -- so
# ask which one to isolate, rather than silently guessing "the first one".
# With only one failure there is nothing to choose, so this step is a
# no-op in that case.
# ---------------------------------------------------------------------------
step "choose which failing testcase to isolate"

if (( ${#fail_idxs[@]} == 1 )); then
   action "check how many testcases failed"
   boundary "only one (${all_names[${fail_idxs[0]}]}) -- Step 2 already" \
            "established this"
   consequence "skipped -- nothing to choose"
   target_idx=${fail_idxs[0]}
else
   action "list every failing testcase, in the order it ran, for the user" \
          "to choose from"
   echo
   for i in "${!fail_idxs[@]}"; do
      fi=${fail_idxs[$i]}
      printf '  %d) %-40s (%s error%s)\n' "$((i + 1))" "${all_names[$fi]}" \
             "${all_errs[$fi]}" \
             "$([[ ${all_errs[$fi]} == 1 ]] || echo s)"
   done
   echo
   boundary "${#fail_idxs[@]} failing testcases printed above, numbered" \
            "1..${#fail_idxs[@]} in run order"

   selection=""
   if (( $# > 1 )) && [[ $2 =~ ^[0-9]+$ ]] \
      && (( $2 >= 1 && $2 <= ${#fail_idxs[@]} ))
   then
      selection=$2
      consequence "selection given on the command line: #$selection"
   # /dev/tty can exist as a device node yet still fail to open (ENXIO --
   # no controlling terminal, e.g. under some sandboxes/containers) even
   # though a plain [[ -r/-w ]] check on it says yes -- confirmed directly,
   # so actually try to open it rather than trust those tests alone. Done
   # in a subshell first, purely to test openability: "exec 5<>/dev/tty
   # 2>/dev/null" directly (no subshell) does NOT suppress bash's own
   # redirection-failure message here -- confirmed directly -- because
   # that message is emitted while setting up 5<>/dev/tty itself, before
   # the later 2>/dev/null redirection in the same command has taken
   # effect. A subshell scopes both the attempt and its stderr cleanly.
   elif ( exec 5<>/dev/tty ) 2>/dev/null && exec 5<>/dev/tty; then
      # read from /dev/tty (fd 5) explicitly, not plain stdin -- later apl
      # runs in this script feed their own stdin via a heredoc, but that
      # must never be confused with (or consume) this interactive prompt,
      # and this also lets the prompt work even if the script's own stdin
      # happens to be redirected from something else.
      while :; do
         read -r -p "Which failing testcase should be isolated? [1-${#fail_idxs[@]}, default 1]: " \
                 selection <&5 >&5 2>&1
         [[ -z "$selection" ]] && { selection=1; break; }
         [[ "$selection" =~ ^[0-9]+$ ]] \
            && (( selection >= 1 && selection <= ${#fail_idxs[@]} )) && break
         echo "please enter a number between 1 and ${#fail_idxs[@]}" >&5
      done
      exec 5>&-
      consequence "user selected #$selection"
   else
      selection=1
      consequence "no usable /dev/tty (non-interactive run) and no" \
                  "selection given on the command line -- defaulting to" \
                  "#1 (${all_names[${fail_idxs[0]}]}); pass a number as a" \
                  "second argument to choose a different one non-interactively"
   fi

   target_idx=${fail_idxs[$((selection - 1))]}
fi

target=${all_names[$target_idx]}
boundary "isolating #$((target_idx + 1)) of ${#all_names[@]}: $target"
consequence "target = $target"

# ---------------------------------------------------------------------------
# 3. drop every testcase that ran after the first failure
# ---------------------------------------------------------------------------
step "drop the testcases that ran after the failure"

candidates=("${all_names[@]:0:target_idx}")
candidate_paths=("${all_paths[@]:0:target_idx}")
target_path=${all_paths[$target_idx]}
skipped=$(( ${#all_names[@]} - target_idx - 1 ))

action "slice the parsed list at $target -- everything after it never ran" \
       "before the failure, so it cannot be a cause of it"
boundary "$skipped testcase(s) ran after $target"
consequence "${#candidates[@]} candidate(s) remain:" "${candidates[*]}"

# ---------------------------------------------------------------------------
# symlink every candidate + the target into a private scratch directory
# once, up front. Trials below always select an explicit subset of these
# fixed symlinks by name -- never a shell wildcard -- so ordering is
# whatever this script's own arrays say it is, not whatever the shell
# glob happens to sort to. This also keeps every apl run's .log/summary.log
# side effects out of the real testcases/ tree.
# ---------------------------------------------------------------------------
rm -rf "$SRC_DIR/tmp"
mkdir -p "$WORK"

for i in "${!candidates[@]}"; do
   ln -s "$SRC_DIR/${candidate_paths[$i]}" "$WORK/${candidates[$i]}"
done
ln -s "$SRC_DIR/$target_path" "$WORK/$target"

# ---------------------------------------------------------------------------
# run_apl_and_check OUTVAR path1 path2 ...
#
# Runs every given testcase path to completion, in exactly the given order
# -- deliberately WITHOUT --TM (which stops the whole run dead at the
# FIRST mismatch anywhere in the set, confirmed directly: with --TM 3, an
# earlier file that itself fails prevents every later file, including
# $target, from ever running at all -- there would be no way to tell
# whether $target's own bug is independent of an earlier known failure, or
# a follow-on effect of it, which is exactly the question Step 3b below
# needs an answer to). Instead, every given path always runs, and this
# looks up $target's OWN recorded result afterward, regardless of what
# else in the run passed or failed, before or after it.
#
# Stores $target's error count (as a plain non-negative integer) into the
# caller's OUTVAR, or the empty string if $target's own result could not
# be determined at all (crashed before its own entry was ever written --
# see the Testfile: banner fallback below). Quiet -- callers narrate the
# result themselves via action/boundary/consequence.
# ---------------------------------------------------------------------------
run_apl_and_check()
{
   local -n out_ref=$1
   shift

   (( ++RUN_COUNT ))

   if (( $# == 0 )); then
      out_ref=""
      return
   fi

   local run_home
   run_home=$(mktemp -d)
   local logfile
   logfile=$(mktemp)
   HOME="$run_home" ./apl --id 1010 --noColor -T "$@" \
      <<< ')OFF' > "$logfile" 2>&1
   rm -rf "$run_home"

   # apl -T always (re)writes a summary.log next to the given testcase
   # paths -- every path this script ever passes lives under $WORK, so
   # that is where to look; parsed with the exact same two-line-entry
   # logic as Step 2 above.
   local errs="" pending="" line path e
   if [[ -f "$WORK/summary.log" ]]; then
      while IFS= read -r line; do
         if [[ "$line" != *.tc ]]; then
            if [[ "$line" =~ ^[[:space:]]*([0-9]+)[[:space:]].*\(.*APL, ]]; then
               pending=${BASH_REMATCH[1]}
            fi
            continue
         fi
         path=${line##* }
         if [[ -n "$pending" ]]; then e=$pending; pending=""
         else read -r e _ <<< "$line"; fi
         [[ "$(basename "$path")" == "$target" ]] || continue
         errs=$e
         break
      done < "$WORK/summary.log"
   fi

   if [[ -z "$errs" ]]; then
      # $target's own entry never made it into summary.log -- apl most
      # likely crashed partway through and never got as far as writing
      # it. The last "Testfile:" banner in the raw transcript names
      # whichever testcase was actually running when that happened.
      local crashed_on
      crashed_on=$(sed -n 's/^ *## *Testfile: *\([^ ]*\.tc\).*$/\1/p' \
                   "$logfile" | tail -1)
      if [[ -n "$crashed_on" ]] && [[ "$(basename "$crashed_on")" == "$target" ]]
      then
         errs="crashed"
      fi
      # otherwise leave errs empty: apl crashed on some OTHER (earlier)
      # file, or exited before reaching $target for some other reason --
      # $target's own status here is simply unknown/not applicable.
   fi

   rm -f "$logfile"
   out_ref=$errs
}

# ---------------------------------------------------------------------------
# reproduces CANDIDATES_ARRAY_NAME -- does this candidate set + target
# still reproduce the SAME failing testcase? Builds the explicit,
# order-preserving path list from $WORK -- no wildcard involved. Sets
# RESULT_TEXT to a human-readable description of what apl actually did,
# for the caller to put in a Boundary Condition box.
# ---------------------------------------------------------------------------
RESULT_TEXT=""

reproduces()
{
   local -n rp_cands=$1
   local rp_paths=()
   local n
   for n in "${rp_cands[@]}"; do rp_paths+=("$WORK/$n"); done
   rp_paths+=("$WORK/$target")

   local rp_errs
   run_apl_and_check rp_errs "${rp_paths[@]}"

   if [[ -z "$rp_errs" ]]; then
      RESULT_TEXT="apl did not reach $target at all -- it crashed (or" \
                  " otherwise stopped) on an earlier file first"
      return 1
   fi

   if [[ "$rp_errs" == crashed ]]; then
      RESULT_TEXT="$target itself crashed apl (terminated abnormally" \
                  " while $target was running)"
      return 0
   fi

   if (( rp_errs > 0 )); then
      RESULT_TEXT="$target still failed ($rp_errs error(s))"
      return 0
   fi

   RESULT_TEXT="apl passed -- $target did not fail"
   return 1
}

# ---------------------------------------------------------------------------
# 3b. first reduction pass: if any OTHER testcase that ran before $target also
# failed (per Step 2/2b), try dropping all of those at once, before doing
# anything else. This is the specific question Step 2b's selection dialog
# raises: is the chosen failure its own, independent bug, or merely
# follow-on damage from one of the other testcases already known to have
# failed earlier in the same run? Bisection/one-by-one removal (Step 6/7
# below) would eventually reach the same answer on its own, but only after
# treating the other known-bad testcase(s) as just more unnamed candidates
# among possibly many passing ones -- asking the question explicitly, first,
# gives a direct answer to exactly what Step 2b's dialog was about, and
# shrinks the set minimization has to work with whenever the answer is
# "independent".
# ---------------------------------------------------------------------------
step "first reduction pass: drop the other known-failing testcase(s)"

other_fail_names=()
for fi in "${fail_idxs[@]}"; do
   (( fi == target_idx )) && continue
   (( fi < target_idx )) && other_fail_names+=("${all_names[$fi]}")
done

action "check whether any OTHER testcase that ran before $target also" \
       "failed, per the summary.log entries parsed in Step 2"
if (( ${#other_fail_names[@]} == 0 )); then
   boundary "no other testcase before $target failed"
   consequence "skipped -- nothing to drop here"
else
   boundary "${#other_fail_names[@]} other failing testcase(s) ran" \
            "before $target: ${other_fail_names[*]}"

   reduced_candidates=()
   for c in "${candidates[@]}"; do
      is_other=no
      for o in "${other_fail_names[@]}"; do
         [[ "$c" == "$o" ]] && { is_other=yes; break; }
      done
      [[ "$is_other" == no ]] && reduced_candidates+=("$c")
   done

   action "run apl on the candidate set with ${#other_fail_names[@]} other" \
          "known-failing testcase(s) removed, keeping every ordinary" \
          "(passing) candidate + $target"
   if reproduces reduced_candidates; then
      boundary "$RESULT_TEXT"
      candidates=("${reduced_candidates[@]}")
      consequence "$target's failure does NOT depend on the other" \
                  "known-failing testcase(s) -- confirmed independent," \
                  "dropping them; ${#candidates[@]} candidate(s) remain" \
                  "for the normal minimization below"
   else
      boundary "$RESULT_TEXT"
      consequence "$target's failure depends on at least one of the" \
                  "other known-failing testcase(s) -- it may be a" \
                  "follow-on effect rather than an independent bug;" \
                  "keeping them in the candidate set so the normal" \
                  "minimization below (which narrows down to exactly" \
                  "what is required) can sort out which one(s)"
   fi
fi

# ---------------------------------------------------------------------------
# 4. sanity check: does the (possibly just-reduced) candidate set still
# reproduce right now?
# ---------------------------------------------------------------------------
step "confirm the failure still reproduces"

action "run apl on all ${#candidates[@]} candidate(s) + $target, and check" \
       "$target's own result"
if reproduces candidates; then
   boundary "$RESULT_TEXT"
   consequence "proceeding to minimize this set"
else
   boundary "$RESULT_TEXT"
   consequence "not reproducible right now -- switching to degraded mode:" \
               "skipping minimization (Step 5/6) and packaging the" \
               "ORIGINAL evidence from $SUMMARY's own run instead. This" \
               "is expected for a flaky/non-deterministic failure (e.g." \
               "a crash inside a third-party library) rather than a" \
               "stale log -- see Step 8/9/10 below."
   DEGRADED=yes
fi

# ---------------------------------------------------------------------------
# 5. bisect: repeatedly try halving the remaining candidate set.
# ---------------------------------------------------------------------------
step "bisect the candidate set"

if [[ "$DEGRADED" == no ]]; then
changed=1
while (( changed && ${#candidates[@]} > 1 )); do
   changed=0
   n=${#candidates[@]}
   mid=$(( (n + 1) / 2 ))

   first_half=("${candidates[@]:0:mid}")
   second_half=("${candidates[@]:mid}")

   action "run apl without the first $mid of $n candidate(s)" \
          "(keeping the last $((n - mid)) + $target)"
   if reproduces second_half; then
      boundary "$RESULT_TEXT"
      candidates=("${second_half[@]}")
      consequence "the first $mid are confirmed unnecessary --" \
                  "${#candidates[@]} candidate(s) left"
      changed=1
      continue
   fi
   boundary "$RESULT_TEXT"
   consequence "the first $mid include something necessary -- trying" \
               "the other half instead"

   action "run apl without the last $((n - mid)) of $n candidate(s)" \
          "(keeping the first $mid + $target)"
   if reproduces first_half; then
      boundary "$RESULT_TEXT"
      candidates=("${first_half[@]}")
      consequence "the last $((n - mid)) are confirmed unnecessary --" \
                  "${#candidates[@]} candidate(s) left"
      changed=1
      continue
   fi
   boundary "$RESULT_TEXT"
   consequence "neither half alone reproduces the failure -- both are" \
               "needed together; switching to one-by-one removal"
done
else
   action "check whether bisection is possible"
   boundary "degraded mode -- Step 4 could not confirm reproduction"
   consequence "skipped -- bisection requires a reliable reproduction" \
               "signal, which is not available here; keeping all" \
               "${#candidates[@]} original candidate(s) unminimized"
fi

# ---------------------------------------------------------------------------
# 6. remove candidates one at a time until every remaining one is
# individually required.
# ---------------------------------------------------------------------------
step "remove candidates one at a time"

if [[ "$DEGRADED" == no ]]; then
i=0
while (( i < ${#candidates[@]} )); do
   trial=("${candidates[@]:0:i}" "${candidates[@]:$((i + 1))}")
   name=${candidates[$i]}

   action "run apl without $name (${#candidates[@]} candidate(s) left" \
          "before this trial)"
   if reproduces trial; then
      boundary "$RESULT_TEXT"
      candidates=("${trial[@]}")
      consequence "$name confirmed unnecessary --" \
                  "${#candidates[@]} candidate(s) left"
   else
      boundary "$RESULT_TEXT"
      consequence "$name is required -- keeping it"
      (( i++ ))
   fi
done
else
   action "check whether one-by-one removal is possible"
   boundary "degraded mode -- Step 4 could not confirm reproduction"
   consequence "skipped -- same reason as Step 5"
fi

# ---------------------------------------------------------------------------
# result
# ---------------------------------------------------------------------------
if [[ "$DEGRADED" == no ]]; then
   step "report the minimal reproducing set"
else
   step "report the (unminimized) original candidate list"
fi

paths=()
for c in "${candidates[@]}"; do paths+=("$WORK/$c"); done
paths+=("$WORK/$target")

if [[ "$DEGRADED" == no ]]; then
   action "summarize: ${#candidates[@]} candidate(s) + $target, $RUN_COUNT" \
          "apl run(s) total"
   consequence "minimal reproducing set:" "${candidates[*]} $target"
else
   action "summarize: degraded mode, $RUN_COUNT apl run(s) total"
   consequence "NOT minimized or independently verified -- this is the" \
               "original candidate list from $SUMMARY, as-is:" \
               "${candidates[*]} $target"
fi

echo
echo "  ${candidates[*]+${candidates[*]}}"
echo "  $target   <- fails"
echo
if [[ "$DEGRADED" == no ]]; then
   echo "Left behind in $WORK for inspection:"
   printf '  HOME=$(mktemp -d) ./apl --id 1010 --TM 3 -T'
   printf ' %s' "${paths[@]}"
   echo
fi

# ---------------------------------------------------------------------------
# 8. build a standalone .apl reproducer. A '----> ' prefix in a .tc.log
# unambiguously means "this line was fed to the interpreter as input"
# (see IO_Files::get_file_line() -- every line read from the test file is
# echoed with that prefix before it is executed; expected-output/diff
# lines are written separately, by a different code path, and never get
# the prefix). So extracting exactly the '----> '-prefixed lines from
# the minimal set's .tc.log files, in order, and stripping the prefix,
# gives the literal raw APL source that was typed -- comments and blank
# lines included, since those are harmless no-ops when replayed too --
# with no need to parse the .tc file's own indentation conventions at
# all. That is a plain, portable .apl script usable outside any
# testcase machinery.
# ---------------------------------------------------------------------------
step "build and verify a standalone .apl reproducer"

APL_FILE=$SRC_DIR/$LOGDIR/culprits.apl
verify_rc=0

if [[ "$DEGRADED" == no ]]; then
   action "re-run apl on exactly the final minimal set, to get" \
          "authoritative .tc.log files for it (earlier trials in Step" \
          "5/6 left stale ones behind for files no longer in the set)"
   if reproduces candidates; then
      boundary "$RESULT_TEXT"
      consequence "$WORK/*.tc.log now reflect exactly this set"
   else
      boundary "$RESULT_TEXT"
      consequence "aborting -- the minimal set stopped reproducing" \
                  "between Step 6 and here"
      echo "Automated_Test_Report.sh: minimal set no longer reproduces" >&2
      exit 1
   fi

   log_paths=()
   for c in "${candidates[@]}"; do log_paths+=("$WORK/$c.log"); done
   target_log_path=$WORK/$target.log
else
   action "locate the ORIGINAL .tc.log files from $SUMMARY's own run --" \
          "degraded mode cannot regenerate fresh ones reliably (that is" \
          "exactly what Step 4 already found does not work right now)"
   log_paths=()
   for p in "${candidate_paths[@]}"; do log_paths+=("$SRC_DIR/$p.log"); done
   target_log_path=$SRC_DIR/$target_path.log
   if [[ -f "$target_log_path" ]]; then
      boundary "$target_log_path exists"
      consequence "building $APL_FILE from the original logs"
   else
      boundary "$target_log_path does not exist either"
      consequence "cannot build $APL_FILE -- no source lines available"
   fi
fi

action "extract the '----> '-prefixed lines from each candidate's and" \
       "$target's .tc.log, in order, and strip the prefix"
if [[ ! -f "$target_log_path" ]]; then
   boundary "$target_log_path is not available"
   consequence "skipped -- $APL_FILE was not built"
   APL_FILE=""
else
   {
      for i in "${!candidates[@]}"; do
         echo "⍝ ═══ ${candidates[$i]} ═══"
         [[ -f "${log_paths[$i]}" ]] && sed -n 's/^----> //p' "${log_paths[$i]}"
      done
      echo "⍝ ═══ $target ═══"
      sed -n 's/^----> //p' "$target_log_path"
   } > "$APL_FILE.new"
   # never overwrite a previous run's culprits.apl in place -- build the
   # new one under a .new name and only replace the old one, atomically,
   # once it is complete (same policy as culprits.log/culprits.tar.gz).
   boundary "wrote $(wc -l < "$APL_FILE.new") line(s)"
   mv -f "$APL_FILE.new" "$APL_FILE"
   consequence "$APL_FILE"
fi

action "run $APL_FILE standalone (apl -f, no -T/testcase machinery) to" \
       "verify it still produces the fault"
if [[ -z "$APL_FILE" ]]; then
   boundary "no $APL_FILE was built"
   consequence "skipped -- nothing to verify"
else

# the known-bad signature: every raw (unmodified) output line that
# target's own .tc.log recorded a mismatch against. DiffOut.cc writes
# 'apl: <marker><raw output line><marker>' with a fixed 3-dot marker
# and no other transformation -- so stripping the marker recovers
# exactly what the interpreter printed, comparable byte-for-byte
# against a fresh run's own raw output.
mapfile -t bad_values < <(sed -n 's/^apl: ⋅⋅⋅\(.*\)⋅⋅⋅$/\1/p' "$target_log_path")

verify_home=$(mktemp -d)
verify_out=$(HOME="$verify_home" ./apl --id 1010 --noColor --TM 3 \
             -f "$APL_FILE" <<< ')OFF' 2>&1)
verify_rc=$?
rm -rf "$verify_home"

reproduced=no
reason=""
if (( verify_rc != 0 )); then
   reproduced=yes
   reason="apl exited with status $verify_rc (abnormal termination)"
else
   for bv in "${bad_values[@]}"; do
      if [[ -n "$bv" ]] && grep -qF "$bv" <<< "$verify_out"; then
         reproduced=yes
         reason="the known-bad value '$bv' reappeared in the output"
         break
      fi
   done
fi

if [[ "$reproduced" == yes ]]; then
   boundary "$reason"
   consequence "verified -- $APL_FILE still reproduces the fault"
else
   boundary "apl exited 0 and none of the ${#bad_values[@]} known-bad" \
            "value(s) recorded for $target reappeared"
   consequence "NOT verified -- shipping $APL_FILE anyway, but flagged" \
               "as unconfirmed"
fi
fi

# ---------------------------------------------------------------------------
# 9. attempt a gdb backtrace of the crash, if Step 8's verification run
# actually crashed (as opposed to reproducing via an ordinary wrong
# output value, which has nothing to backtrace).
# ---------------------------------------------------------------------------
step "attempt a gdb backtrace of any crash"

GDB_FILE=""
BACKTRACE_FILE=""

if [[ "$DEGRADED" == yes ]]; then
   action "degraded mode: a fresh gdb run would be just as unreliable as" \
          "Step 4's own reproduction attempt -- check for a backtrace" \
          "the interpreter itself already persisted to disk at the" \
          "original failure's crash time instead (see" \
          "Backtrace::show_signal_safe(), IO_Files::report_abnormal_exit())"
   persisted=$SRC_DIR/$target_path.crash.txt
   if [[ -f "$persisted" ]]; then
      boundary "$persisted exists"
      BACKTRACE_FILE=$SRC_DIR/$LOGDIR/culprits_backtrace.txt
      cp "$persisted" "$BACKTRACE_FILE.new"
      mv -f "$BACKTRACE_FILE.new" "$BACKTRACE_FILE"
      consequence "using the interpreter's own crash-time backtrace:" \
                  "$BACKTRACE_FILE"
   else
      boundary "$persisted does not exist (either an older interpreter" \
               "build, or the original failure was not a crash/FIXME)"
      consequence "no backtrace available for this run"
   fi
fi

if [[ "$DEGRADED" == no ]]; then

action "check whether Step 8's verification run actually crashed --" \
       "only that is worth a gdb backtrace; an ordinary wrong-output" \
       "reproduction has no crash site to look at"
if (( verify_rc != 0 )); then
   boundary "the verification run exited with status $verify_rc"
   consequence "attempting to gather a gdb backtrace"
else
   boundary "the verification run exited 0"
   consequence "skipping -- nothing to backtrace"
fi

if (( verify_rc != 0 )); then

   action "check whether gdb is available"
   if gdb_path=$(command -v gdb); then
      boundary "found $gdb_path"
      consequence "proceeding"
      have_gdb=yes
   else
      boundary "gdb is not installed / not in \$PATH"
      consequence "skipping -- install gdb to enable backtrace collection"
      have_gdb=no
   fi

   if [[ "$have_gdb" == yes ]]; then

      action "check whether core dumps are enabled (ulimit -c), for the" \
             "fallback path below"
      soft_c=$(ulimit -Sc)
      hard_c=$(ulimit -Hc)
      if [[ "$soft_c" == unlimited || "$soft_c" != 0 ]]; then
         boundary "the core dump soft limit is already '$soft_c'"
         consequence "no change needed"
      elif [[ "$hard_c" == unlimited || "$hard_c" != 0 ]]; then
         ulimit -Sc unlimited
         boundary "the soft limit was 0, but the hard limit ('$hard_c')" \
                  "allows raising it"
         consequence "raised the soft limit to unlimited for this session"
      else
         boundary "both the soft and hard core-dump limits are 0 --" \
                  "this script cannot raise the hard limit itself" \
                  "(needs root, or a shell/systemd configuration change)"
         consequence "please run, as root: ulimit -Hc unlimited (or add" \
                     "'* hard core unlimited' to" \
                     "/etc/security/limits.conf and re-login) if you" \
                     "want core-file-based backtraces to be possible"
      fi

      action "decide how to get the backtrace: run apl directly under" \
             "gdb, or run it standalone and load the resulting core file"
      core_pattern=$(cat /proc/sys/kernel/core_pattern 2>/dev/null)
      boundary "running apl AS A CHILD OF gdb is more reliable here:" \
               "apl installs its own SIGSEGV handler (main.cc) that" \
               "catches a real segfault and exits gracefully -- that" \
               "suppresses the OS's own core dump entirely for exactly" \
               "the crashes we care about, and this system's" \
               "core_pattern ('$core_pattern') pipes to a coredump" \
               "collector rather than writing a plain file, which a" \
               "standalone run would also have to account for. gdb" \
               "gets first look at the signal (via ptrace) regardless" \
               "of any handler the program itself installs."
      consequence "running apl directly under gdb first; falling back" \
                  "to a standalone run + core file only if that" \
                  "doesn't catch anything"

      action "run $APL_FILE under gdb (run; bt full) and capture the" \
             "session"
      GDB_FILE=$SRC_DIR/$LOGDIR/culprits_gdb.txt
      gdb_home=$(mktemp -d)
      HOME="$gdb_home" gdb --batch -q \
         -ex 'set pagination off' \
         -ex run \
         -ex 'bt full' \
         -ex kill \
         -ex quit \
         --args ./apl -q --noColor --TM 3 -f "$APL_FILE" \
         <<< ')OFF' > "$GDB_FILE.new" 2>&1
      rm -rf "$gdb_home"

      # write to a .new scratch file and only ever promote it onto the
      # user-facing culprits_gdb.txt on success -- an inconclusive gdb
      # session here must never delete or truncate a previous run's
      # genuine backtrace still sitting at that name.
      if grep -q '^#0 ' "$GDB_FILE.new"; then
         mv -f "$GDB_FILE.new" "$GDB_FILE"
         boundary "gdb's session shows a live stack (a '#0' frame is" \
                  "present) -- it stopped on a real signal"
         consequence "backtrace captured in $GDB_FILE"
      else
         boundary "no '#0' frame in gdb's output -- apl ran to normal" \
                  "completion under gdb without gdb stopping it on a" \
                  "signal this time"
         rm -f "$GDB_FILE.new"   # our own scratch file, never $GDB_FILE
         GDB_FILE=""

         if (( verify_rc >= 128 )); then
            consequence "status $verify_rc means an unhandled signal" \
                        "killed it earlier -- trying a standalone run" \
                        "+ core file instead"

            action "run $APL_FILE standalone (core dumps enabled) and" \
                   "look for a resulting core"
            core_home=$(mktemp -d)
            ( ulimit -Sc unlimited 2>/dev/null
              HOME="$core_home" ./apl -q --noColor --TM 3 -f "$APL_FILE" \
                 <<< ')OFF' > /dev/null 2>&1 )
            rm -rf "$core_home"

            GDB_FILE=$SRC_DIR/$LOGDIR/culprits_gdb.txt
            # as above: everything is captured under a .new scratch name
            # first; only a successful capture is ever promoted onto the
            # user-facing culprits_gdb.txt, so a failed attempt here can
            # never delete or truncate a previous run's real backtrace.
            if command -v coredumpctl > /dev/null 2>&1 \
               && coredumpctl gdb --quiet -1 "$SRC_DIR/apl" \
                  -- -batch -ex 'bt full' -ex quit \
                  > "$GDB_FILE.new" 2>&1 \
               && grep -q '^#0 ' "$GDB_FILE.new"
            then
               mv -f "$GDB_FILE.new" "$GDB_FILE"
               boundary "coredumpctl found a matching core (this" \
                        "system's core_pattern pipes to" \
                        "systemd-coredump)"
               consequence "backtrace captured in $GDB_FILE"
            else
               rm -f "$GDB_FILE.new"
               core_file=$(find "$SRC_DIR" -maxdepth 1 \
                           -newer "$APL_FILE" \( -name 'core' -o \
                           -name 'core.*' \) -print -quit 2>/dev/null)
               if [[ -n "$core_file" ]] \
                  && gdb --batch -q -ex 'bt full' -ex quit \
                     ./apl "$core_file" > "$GDB_FILE.new" 2>&1 \
                  && grep -q '^#0 ' "$GDB_FILE.new"
               then
                  mv -f "$GDB_FILE.new" "$GDB_FILE"
                  boundary "found a plain core file: $core_file"
                  consequence "backtrace captured in $GDB_FILE"
               else
                  rm -f "$GDB_FILE.new"
                  boundary "neither coredumpctl nor a plain core file" \
                           "in $SRC_DIR yielded a usable backtrace"
                  consequence "no gdb backtrace available for this run"
                  GDB_FILE=""
               fi
            fi
         else
            consequence "status $verify_rc is one of this interpreter's" \
                        "own graceful abnormal-exit codes (not an" \
                        "unhandled signal) -- it already printed its" \
                        "own backtrace at the time, visible above; no" \
                        "core dump would exist for it either way"
         fi
      fi
   fi
fi

fi   # DEGRADED == no

# ---------------------------------------------------------------------------
# 10. package diagnostic artifacts into a tarball the user can email us.
#
# tar can append to an existing (uncompressed) archive but not to an
# already-gzipped one, so the archive is built up file by file with
# plain 'tar -rf' (or '-cf' for the first file) and only gzipped once,
# as the very last step -- more artifacts can be added the same way
# later without needing to touch the compression step at all.
#
# culprits.tar.gz (FINAL_TARBALL) is the user-facing artifact from a
# previous run and must never be deleted or left in a half-written state
# -- so the new archive is built entirely under a .new name (TARFILE) and
# only the finished culprits.tar.gz.new is renamed onto culprits.tar.gz,
# atomically, once compression has actually succeeded.
# ---------------------------------------------------------------------------
step "package diagnostic artifacts for reporting"

TARFILE=$SRC_DIR/$LOGDIR/culprits.tar.new
FINAL_TARBALL=$SRC_DIR/$LOGDIR/culprits.tar.gz
rm -f "$TARFILE" "$TARFILE.gz"

# add_artifact SRC_PATH -- append SRC_PATH (by basename) to $TARFILE,
# creating it on the first call.
add_artifact()
{
   local src=$1
   if [[ -f "$TARFILE" ]]; then
      tar -rf "$TARFILE" -C "$(dirname "$src")" "$(basename "$src")"
   else
      tar -cf "$TARFILE" -C "$(dirname "$src")" "$(basename "$src")"
   fi
}

target_log=$SRC_DIR/$LOGDIR/$target.log

action "add $target.log, the failing testcase's own diff/output log," \
       "recorded by the original run"
if [[ -f "$target_log" ]]; then
   boundary "$target_log exists"
   add_artifact "$target_log"
   consequence "added to $TARFILE"
else
   boundary "$target_log does not exist"
   consequence "skipped -- nothing to add"
fi

action "add culprits.apl, the standalone reproducer built in Step 8"
if [[ -f "$APL_FILE" ]]; then
   boundary "$APL_FILE exists"
   add_artifact "$APL_FILE"
   consequence "added to $TARFILE"
else
   boundary "$APL_FILE does not exist"
   consequence "skipped -- nothing to add"
fi

action "add culprits_gdb.txt, the gdb backtrace built in Step 9 (if any)"
if [[ -n "$GDB_FILE" && -f "$GDB_FILE" ]]; then
   boundary "$GDB_FILE exists"
   add_artifact "$GDB_FILE"
   consequence "added to $TARFILE"
else
   boundary "no gdb backtrace was captured in Step 9"
   consequence "skipped -- nothing to add"
fi

action "add culprits_backtrace.txt, the interpreter's own persisted" \
       "crash-time backtrace found in Step 9 (degraded mode only)"
if [[ -n "$BACKTRACE_FILE" && -f "$BACKTRACE_FILE" ]]; then
   boundary "$BACKTRACE_FILE exists"
   add_artifact "$BACKTRACE_FILE"
   consequence "added to $TARFILE"
else
   boundary "not applicable (normal mode, or no persisted backtrace" \
            "was found)"
   consequence "skipped -- nothing to add"
fi

SUMMARY_COPY=""

action "add culprits_summary.log, a copy of $SUMMARY (degraded mode" \
       "only) -- context for exactly where and how badly the target" \
       "failed in the original run, since nothing here was" \
       "independently re-confirmed"
if [[ "$DEGRADED" == yes ]]; then
   SUMMARY_COPY=$SRC_DIR/$LOGDIR/culprits_summary.log
   cp "$SUMMARY" "$SUMMARY_COPY.new"
   mv -f "$SUMMARY_COPY.new" "$SUMMARY_COPY"
   boundary "$SUMMARY exists (checked in Step 1)"
   add_artifact "$SUMMARY_COPY"
   consequence "added to $TARFILE as $(basename "$SUMMARY_COPY")"
else
   boundary "normal mode -- the minimal set was independently reproduced" \
            "and verified above"
   consequence "skipped -- nothing to add"
fi

action "add culprits_env.txt, environment/library-version info --" \
       "useful when the fault lives in third-party library code (GTK," \
       "GDK, glib, X11, ...) rather than in GNU APL's own"
ENV_FILE=$SRC_DIR/$LOGDIR/culprits_env.txt
{
   echo "=== uname -a ==="
   uname -a 2>&1
   echo
   echo "=== \$DISPLAY ==="
   echo "${DISPLAY:-<unset>}"
   echo
   echo "=== ./apl --cfg ==="
   ./apl --cfg 2>&1
   echo
   echo "=== GTK/GDK/glib/X11/cairo/pango versions (pkg-config) ==="
   if command -v pkg-config > /dev/null 2>&1; then
      for pkg in gtk+-3.0 gdk-3.0 glib-2.0 gobject-2.0 cairo pango x11; do
         pkg-config --exists "$pkg" 2>/dev/null \
            && printf '%-14s %s\n' "$pkg" "$(pkg-config --modversion "$pkg")"
      done
   else
      echo "(pkg-config not available)"
   fi
   echo
   echo "=== relevant installed packages (dpkg, if available) ==="
   if command -v dpkg-query > /dev/null 2>&1; then
      dpkg-query -W -f='${Package} ${Version}\n' 2>/dev/null \
         | grep -E '^(libgtk|libgdk|libglib|libx11|libcairo|libpango)' \
         | sort
   else
      echo "(dpkg-query not available)"
   fi
} > "$ENV_FILE.new" 2>&1
mv -f "$ENV_FILE.new" "$ENV_FILE"
boundary "wrote $ENV_FILE"
add_artifact "$ENV_FILE"
consequence "added to $TARFILE"

action "add culprits.log, this run's own narration, to the tar"
boundary "this is the last line culprits.log will contain -- it can" \
         "only be archived once it is done being written to"
consequence "added to $TARFILE"

# stop logging to culprits.log.new now, right after narrating (and before
# performing) its own archival -- so the copy inside the tar is complete
# up to and including that consequence line, instead of being truncated
# mid-sentence by the archival step itself. Everything from here on is
# terminal-only (fd 3/4, saved before logging started). Closing fd 1/2
# sends the 'tee' process (still draining the pipe in the background) an
# EOF; 'wait' blocks until it has actually flushed and exited, so the
# file on disk is guaranteed complete before it gets renamed and tar'd up
# -- without this, tar could race tee and archive a truncated file.
exec 1>&3 2>&4
wait

# atomically replace the previous culprits.log (if any) with the
# complete new one -- never leaves culprits.log missing or truncated,
# even if something below this point were to fail.
mv -f "$CULPRITS_LOG.new" "$CULPRITS_LOG"
add_artifact "$CULPRITS_LOG"

action "compress $TARFILE and replace culprits.tar.gz"
gzip -f "$TARFILE"

# only now, with the new archive fully built and compressed, replace the
# previous culprits.tar.gz (if any) -- atomically, and only on success,
# so a previous run's artifact is never deleted or left half-written.
mv -f "$TARFILE.gz" "$FINAL_TARBALL"
consequence "artifacts ready at $FINAL_TARBALL"

# ---------------------------------------------------------------------------
# 11. remove the loose per-artifact files -- everything they contain now
# also lives inside $FINAL_TARBALL, so keeping separate copies around is
# just clutter. This happens only now, after $FINAL_TARBALL has been
# built and renamed into place successfully, so nothing is ever removed
# without first being safely captured in the tarball. culprits.log and
# $FINAL_TARBALL themselves are never touched here -- those two are the
# only artifacts this script ever leaves on disk, and (as elsewhere in
# this script) neither is ever deleted, only atomically replaced with a
# newer version.
#
# This narration happens after logging to culprits.log has already
# stopped (see above -- culprits.log can only record what happened
# before it was itself archived), so it is terminal-only.
# ---------------------------------------------------------------------------
step "remove now-redundant loose files"

action "delete the loose copies of everything just packaged above --" \
       "their content survives inside $FINAL_TARBALL"
removed=()
for f in "$APL_FILE" "$GDB_FILE" "$BACKTRACE_FILE" "$ENV_FILE" "$SUMMARY_COPY"; do
   if [[ -n "$f" && -f "$f" ]]; then
      rm -f "$f"
      removed+=("$(basename "$f")")
   fi
done
if (( ${#removed[@]} )); then
   boundary "removed: ${removed[*]}"
else
   boundary "nothing to remove"
fi
consequence "$CULPRITS_LOG and $FINAL_TARBALL are the only artifacts" \
            "left on disk"

echo
echo "Artifacts for emailing to bug-apl@gnu.org: $FINAL_TARBALL"
