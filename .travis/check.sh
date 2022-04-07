version_less_equal() {
  [ "$1" = "`echo -e "$1\n$2" | sort -V | head -n1`" ]
}

version_less() {
  [ "$1" = "$2" ] && return 1 || version_less_equal $1 $2
}

if [[ -z "${PLATFORMIO_AUTH_TOKEN}" ]]
then
  echo "PLATFORMIO_AUTH_TOKEN is empty"
  exit 1
fi

OUTPUT=$(pio account show 2>&1)

if [[ "${OUTPUT:0:5}" == "Error" ]]; then
  echo "Invalid auth token!"
  exit 1
fi

echo "Valid auth token..."

LIBRARY_VERSION=$(cat library.json | jq '.version')

echo "Library version is ${LIBRARY_VERSION}"

PIO_JSON=$(pio lib show codewitch-honey-crisis/htcw_gfx --json-output)

LATEST_VERSION=$(echo ${PIO_JSON} | jq '.version.name')

echo "Latest published version is ${LATEST_VERSION}"

version_less LIBRARY_VERSION LATEST_VERSION && echo "No need to publish" && exit 1

echo "Publishing..."
exit 0
