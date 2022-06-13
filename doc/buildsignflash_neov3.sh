# exit when any command fails
set -e
echo "Version $1 set for this image"
west build -p auto -b waltherneov3 -d build/waltherneov3 -- -DBOARD_ROOT=samples/bluetooth/adcmesh_demo/ -DOVERLAY_CONFIG=overlay-mcuboot.conf samples/bluetooth/adcmesh_demo
west sign --tool-path ../bootloader/mcuboot/scripts/imgtool.py -t imgtool -d build/waltherneov3 -- --key samples/bluetooth/adcmesh_demo/key/ipdneoprivatekey.pem --version $1
west flash --runner jlink -d build/waltherneov3/ --bin-file build/waltherneov3/zephyr/zephyr.signed.bin
