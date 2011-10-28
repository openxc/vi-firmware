all: can_decoder_c346_hs.pde can_decoder_c346_ms.pde can_decoder_c346_info.pde can_decoder_steering_wheel_test.pde

build_directory:
	mkdir -p build

can_decoder_c346_hs.pde: c346_hs_can.json generate_code.py build_directory
	./generate_code.py build/c346_hs_can.json -p 7 10 > build/can_decoder_c346_hs.pde

can_decoder_c346_ms.pde: c346_ms_can.json generate_code.py build_directory
	./generate_code.py build/c346_ms_can.json > build/can_decoder_c346_ms.pde

can_decoder_c346_info.pde: c346_info_can.json generate_code.py build_directory
	./generate_code.py build/c346_info_can.json -p 30 > build/can_decoder_c346_info.pde

can_decoder_steering_wheel_test.pde: generate_code.py build_directory
	./generate_code.py ../cansignals/c346_steering_wheel_test.json > build/can_decoder_steering_wheel_test.pde

c346_hs_can.json: ../cansignals/c346_hs_can.xml ../cansignals/c346_hs_mapping.txt xml_to_json.py build_directory
	./xml_to_json.py ../cansignals/c346_hs_can.xml ../cansignals/c346_hs_mapping.txt build/c346_hs_can.json

c346_ms_can.json: ../cansignals/c346_ms_can.xml ../cansignals/c346_ms_mapping.txt xml_to_json.py build_directory
	./xml_to_json.py ../cansignals/c346_ms_can.xml ../cansignals/c346_ms_mapping.txt build/c346_ms_can.json

c346_info_can.json: ../cansignals/c346_info_can.xml ../cansignals/c346_info_mapping.txt xml_to_json.py build_directory
	./xml_to_json.py ../cansignals/c346_info_can.xml ../cansignals/c346_info_mapping.txt build/c346_info_can.json

clean:
	rm -rf build
