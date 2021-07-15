//installscript.qs
function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    component.addOperation("Execute", "cmd", "/k", "cd", "/d", "@TargetDir@", "&", "modify.exe", "&", "exit");
}