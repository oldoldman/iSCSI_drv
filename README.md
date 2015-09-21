# iSCSI_drv
an iSCSI demo driver for Windows

1. components
-------------
<table>
<tr><td bgcolor="d8d8d8"><strong>component</strong></td><td><strong>description</strong></td></tr>
<tr><td>uSCSIPort.sys</td><td>iSCSI protocol lib</td></tr>
<tr><td>uSCSI.sys</td><td>disk driver</td></tr>
<tr><td>uscsi.inf</td><td>inf file</td></tr>
<tr><td>devcon</td><td>driver tools</td></tr>
<tr><td>uscsictl</td><td>command line tools , use to add target , connect initiator to target etc.</td></tr>
</table>

2. installation
-------------
1. copy uSCSIPort.sys to system32/drivers
1. install disk driver

 **devcon install uscsi.inf uSCSI\uSCSI**

1. setup StarWind with target name iqn.com.yushang:vdisk.01
1. add target

 **uscsictl -t iqn.com.yushang:vdisk.01 192.168.1.123**

1. connect to target 

 **uscsictl -c iqn.com.yushang:vdisk.01**

1. initialize disk use Windows disk management MMC

3. performance metrics
----------------------
on MS VPC , test with HD Tune
* READ:
 * min 2.8  m/s
 * avg 15.2 m/s
 * max 20.1 m/s
 * access time 11.1 ms
 * burst rate 19.2 m/s
 * CPU usage 14.3%

4. reference
------------
please refer uSCSI设计文档.pdf(Chinese) for details



