TEMPLATE = subdirs
SUBDIRS = \
    portdiscovery \
    machineinfo \
    machinecommunication \
    gcodesender \
    testcommon \
    wirecontroller \
    machinestate \
    machinestatusmonitor \
    commandsender \
    localshapesfinder \
    shapeinfo

portdiscovery.depends = testcommon
machineinfo.depends = testcommon
machinecommunication.depends = testcommon
gcodesender.depends = testcommon
wirecontroller.depends = testcommon
machinestate.depends = testcommon
machinestatusmonitor.depends = testcommon
commandsender.depends = testcommon
localshapesfinder.depends = testcommon
shapeinfo.depends = testcommon
