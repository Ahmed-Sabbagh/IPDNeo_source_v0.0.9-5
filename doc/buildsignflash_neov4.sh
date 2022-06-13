# exit when any command fails
set -e
echo "Version $1 set for this image"
west build -p auto -b waltherneov4 -d build/waltherneov4 -- -DBOARD_ROOT=samples/bluetooth/adcmesh_demo/ -DOVERLAY_CONFIG=overlay-mcuboot.conf samples/bluetooth/adcmesh_demo
west sign --tool-path ../bootloader/mcuboot/scripts/imgtool.py -t imgtool -d build/waltherneov4 -- --key samples/bluetooth/adcmesh_demo/key/ipdneoprivatekeyv4.pem --version $1
west flash --runner jlink -d build/waltherneov4/ --bin-file build/waltherneov4/zephyr/zephyr.signed.bin
