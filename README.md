# iSCSI_drv
an iSCSI demo driver for Windows
1. components
component           description
--------------------------------------------------------
uSCSIPort.sys       iSCSI protocol lib
uSCSI.sys           disk driver
uscsi.inf           inf file
devcon              driver tools
uscsictl            command tools , use to add target , 
                    connect initiator to target etc.
--------------------------------------------------------
2. installation
2.1 copy uSCSIPort.sys to system32/drivers
2.2 install disk driver
devcon install uscsi.inf uSCSI\uSCSI
2.3 setup StarWind
2.4 add target
uscsictl -t iqn.com.yushang:vdisk.01 192.168.1.123
2.5 connect to target 
uscsictl -c iqn.com.yushang:vdisk.01
2.6 initialize disk use Windows disk management MMC
