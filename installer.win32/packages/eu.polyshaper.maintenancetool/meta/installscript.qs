function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/maintenancetool.exe", "@StartMenuDir@/MaintenanceTool.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/maintenancetool.exe", "iconId=0", "description=PolyShaper Maintenance Tool");
    }
}
