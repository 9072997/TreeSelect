open System
open System.Windows.Forms
open System.Text.Json
open System.Drawing

// Function to recursively add nodes to the TreeView based on the JSON structure
let rec addNodes (element: JsonElement) (parentNode: TreeNodeCollection) =
    match element.ValueKind with
    | JsonValueKind.Object ->
        for prop in element.EnumerateObject() do
            let childNode = parentNode.Add(prop.Name)
            addNodes prop.Value childNode.Nodes
    | JsonValueKind.Array ->
        for item in element.EnumerateArray() do
            // For arrays, we add a placeholder node as arrays don't have keys
            let childNode = parentNode.Add("[]")
            addNodes item childNode.Nodes
    | _ -> () // For simplicity, we don't add leaf values as nodes

// Function to create and show the TreeView dialog
let showDialog (json: string) (windowTitle: string) =
    let form = new Form(Width = 300, Height = 400)
    form.Text <- windowTitle
    
    // Set icon
    let assembly = System.Reflection.Assembly.GetExecutingAssembly()
    let resource = assembly.GetManifestResourceStream("TreeSelect.icon.ico")
    let icon = new Icon(resource)
    form.Icon <- icon

    // Create a TreeView control and add it to the form
    let treeView = new TreeView(Dock = DockStyle.Fill)
    form.Controls.Add(treeView)

    // Parse the JSON and populate the TreeView
    use doc = JsonDocument.Parse(json)
    addNodes doc.RootElement treeView.Nodes

    // Variable to hold the selected path
    let mutable selectedPath = ""

    // Event handler for node selection
    treeView.AfterSelect.Add(fun args ->
        let node = args.Node

        let pathList =
            List.fold
                (fun acc (n: TreeNode) -> n.Text :: acc)
                []
                (Seq.unfold (fun (n: TreeNode) -> if n = null then None else Some(n, n.Parent)) node
                 |> Seq.toList)

        selectedPath <- String.Join("/", pathList))

    // Create an OK button
    let okButton = new Button(Text = "OK", Dock = DockStyle.Bottom)

    okButton.Click.Add(fun _ ->
        form.DialogResult <- DialogResult.OK
        form.Close())

    // Add the OK button to the form
    form.Controls.Add(okButton)

    // Set the form's AcceptButton property
    form.AcceptButton <- okButton

    // Show the dialog
    let dialogResult = form.ShowDialog()

    // Check if the user clicked OK and print the selected path
    if dialogResult = DialogResult.OK then
        Console.Write(selectedPath)

// Main function to read JSON from stdin and show the dialog
[<EntryPoint>]
let main argv =
    let json = Console.In.ReadToEnd()
    // Check if there are any command line arguments and concatenate them for the title
    let windowTitle =
        if argv.Length > 0 then
            String.Join(" ", argv)
        else
            "TreeSelect"

    showDialog json windowTitle
    0
