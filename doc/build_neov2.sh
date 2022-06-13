west build -p auto -b waltherneov2 -d build/waltherneov2 -- -DBOARD_ROOT=samples/bluetooth/adcmesh_demo/ -DOVERLAY_CONFIG=overlay-logging.conf samples/bluetooth/adcmesh_demo
west flash --runner jlink -d build/waltherneov2/
