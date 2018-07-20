function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/ShaCo.exe", "@StartMenuDir@/ShaCo.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/ShaCo.ico", "iconId=0", "description=Run PolyShaper ShaCo");
    }
}
