west build -p auto -b waltherneov4 -d build/waltherneov4 -- -DBOARD_ROOT=samples/bluetooth/adcmesh_demo/ -DOVERLAY_CONFIG=overlay-logging.conf samples/bluetooth/adcmesh_demo
west flash --runner jlink -d build/waltherneov4/
