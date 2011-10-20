all: can_decoder_c346_hs.pde can_decoder_c346_ms.pde can_decoder_c346_info.pde

can_decoder_c346_hs.pde: c346_hs_can.hex hex_to_code.py
	./hex_to_code.py c346_hs_can.hex -p 7 10 > can_decoder_c346_hs.pde

can_decoder_c346_ms.pde: c346_ms_can.hex hex_to_code.py
	./hex_to_code.py c346_ms_can.hex > can_decoder_c346_ms.pde

can_decoder_c346_info.pde: c346_info_can.hex hex_to_code.py
	./hex_to_code.py c346_info_can.hex -p 30 > can_decoder_c346_info.pde


c346_hs_can.hex: ../cansignals/c346_hs_can.xml ../cansignals/c346_hs_mapping.txt xml_to_db.py
	./xml_to_db.py ../cansignals/c346_hs_can.xml ../cansignals/c346_hs_mapping.txt c346_hs_can.hex

c346_ms_can.hex: ../cansignals/c346_ms_can.xml ../cansignals/c346_ms_mapping.txt xml_to_db.py
	./xml_to_db.py ../cansignals/c346_ms_can.xml ../cansignals/c346_ms_mapping.txt c346_ms_can.hex

c346_info_can.hex: ../cansignals/c346_info_can.xml ../cansignals/c346_info_mapping.txt xml_to_db.py
	./xml_to_db.py ../cansignals/c346_info_can.xml ../cansignals/c346_info_mapping.txt c346_info_can.hex

clean:
	rm *.hex *.pde
