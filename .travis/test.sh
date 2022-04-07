echo "Running pre-publish tests..."

pio test -v
TEST_RESULT=$?

if [[ "${TEST_RESULT}" != "0" ]]; then
  echo "One or more tests failed. Aborting."
  exit 1
fi
