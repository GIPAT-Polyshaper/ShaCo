TEMPLATE = subdirs
SUBDIRS = \
    portdiscovery \
    machineinfo \
    machinecommunication \
    gcodesender \
    testcommon \
    wiretemperaturecontroller

portdiscovery.depends = testcommon
machineinfo.depends = testcommon
machinecommunication.depends = testcommon
gcodesender.depends = testcommon
