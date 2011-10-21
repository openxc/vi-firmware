all: can_decoder_c346_hs.pde can_decoder_c346_ms.pde can_decoder_c346_info.pde can_decoder_steering_wheel_test.pde

build_directory:
	mkdir -p build

can_decoder_c346_hs.pde: c346_hs_can.hex generateCode.py build_directory
	./generateCode.py --hex build/c346_hs_can.hex -p 7 10 > build/can_decoder_c346_hs.pde

can_decoder_c346_ms.pde: c346_ms_can.hex generateCode.py build_directory
	./generateCode.py --hex build/c346_ms_can.hex > build/can_decoder_c346_ms.pde

can_decoder_c346_info.pde: c346_info_can.hex generateCode.py build_directory
	./generateCode.py --hex build/c346_info_can.hex -p 30 > build/can_decoder_c346_info.pde

can_decoder_steering_wheel_test.pde: generateCode.py build_directory
	./generateCode.py --json ../cansignals/c346_steering_wheel_test.json > build/can_decoder_steering_wheel_test.pde

c346_hs_can.hex: ../cansignals/c346_hs_can.xml ../cansignals/c346_hs_mapping.txt xml_to_db.py build_directory
	./xml_to_db.py ../cansignals/c346_hs_can.xml ../cansignals/c346_hs_mapping.txt build/c346_hs_can.hex

c346_ms_can.hex: ../cansignals/c346_ms_can.xml ../cansignals/c346_ms_mapping.txt xml_to_db.py build_directory
	./xml_to_db.py ../cansignals/c346_ms_can.xml ../cansignals/c346_ms_mapping.txt build/c346_ms_can.hex

c346_info_can.hex: ../cansignals/c346_info_can.xml ../cansignals/c346_info_mapping.txt xml_to_db.py build_directory
	./xml_to_db.py ../cansignals/c346_info_can.xml ../cansignals/c346_info_mapping.txt build/c346_info_can.hex

clean:
	rm -rf build
