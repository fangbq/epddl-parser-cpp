#!/bin/bash

test_cases_file="../epddl/test_cases"
program="./start"

cat $test_cases_file | while read oneline
do
    $program $oneline
    echo
done

exit 0