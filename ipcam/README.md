# ipcamera tools
| Trendnet  | D-Link | Eneo | Unpacker script | Notes |
| ------------- | ------------- | ------------- | ------------- | ------------- |
| TV-IP572P, TV-IP572PI, TV-IP572W, TV-IP572WI, TV-IP672P, TV-IP672PI, TV-IP672W, TV-IP672WI | DCS-942L, DCS-5211L, DCS-5222L Rev. A | | [unp_fw_TV-IP572PI.sh](unp_fw_TV-IP572PI.sh)| Seem to be rebranded Alphanetworks cameras |
| TV-IP512P, TV-IP512WN, TV-IP522P, TV-IP612P, TV-IP612WN | DCS-2121, DCS-3430, DCS-3411, DCS-5635, DCS-5605 | | [unp_fw_DCS-56x5.sh](unp_fw_DCS-56x5.sh) | Seem to be rebranded Alphanetworks cameras |
| TV-IP551WI, TV-IP551W, TV-IP651WI, TV-IP651W | DCS-930L, DCS-931L, DCS-932L	| | [unp_fw_DSC93x.sh](unp_fw_DSC93x.sh) | Firmware contains an uimage including an lzma-compressed data which contains an lzma-compressed romfs starting at a fixed aligned boundary. |
| | 	DCS-3110, DCS-6111 | FLC-1301, FLD-1101 | [unp_fw_DCS-6111.sh](unp_fw_DCS-6111.sh) | Firmware contains a gzipped file named initrd. |
| TV-IP110, TV-IP110W, TV-IP110WN, TV-IP121W, TV-IP121WN, TV-IP212, TV-IP212W, TV-IP252P, TV-IP262P, TV-IP312W, TV-IP312WN, TV-IP322P, TV-IP410, TV-IP410W, TV-IP410WN, TV-IP422, TV-IP422W, TV-IP422WN, TV-VS1, TV-VS1P | | | [fwunpack.pl](fwunpack.pl) | Firmware contains a gzipped minix-image named rootfs. |
| | DCS-2130, DCS-2132L, DCS-2210, DCS-2230, DCS-2330L, DCS-2332L, DCS-3710, DCS-3716, DCS-5222L Rev. B, DCS-6511, DCS-6616, DCS-6815, DCS-6818, DVS-210, DVS-310 | GXC-1710M (=APPRO LC7513), GXB-1710M/IR, GXC-1720M | [decode_fw.c](decode_fw.c), [mount_jffs2.sh](decode_fw.c) | This also applies to American Dynamics cameras, ADCi400-xxxx
| | DNR-202L | 	| [appro_decrypt.c](appro_decrypt.c), [appro_unpack.c](appro_unpack.c) | This also applies to cameras based on Appro DMS-3011, DMS-3014, LC-7211, LC-7213, LC-7214, LC-7215, NVR-2018, NVR-2028, DMS-3016, PVR-3031, LC-7224, LC-7225, DMS-3009, DMS-3004 |
| | | PXD-2018 PTZ1080, PXC-2080 All Level1 cameras | [unp_fw_PXD-2018.sh](unp_fw_PXD-2018.sh), [mount_ubifs.sh](mount_ubifs.sh)	| Firmware contains a UBIFS image. |
