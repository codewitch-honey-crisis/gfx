if [[ -z "${PLATFORMIO_AUTH_TOKEN}" ]]
then
  echo "PLATFORMIO_AUTH_TOKEN is empty, skipping publish"
  exit 0
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

if [[ "${LATEST_VERSION}" = "${LIBRARY_VERSION}" ]]; then
  echo "No need to publish!"
  exit 0
fi

pio package publish --non-interactive
