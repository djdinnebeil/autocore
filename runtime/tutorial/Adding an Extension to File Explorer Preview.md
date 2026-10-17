# Register a Custom File Extension for Windows Text Preview

Use these steps to make a custom file extension preview in Windows File Explorer the same way as a `.txt` file.

Replace:

```text
.<extension>
```

with the file extension you want to register, such as:

```text
.list
.map
.event
```

## 1. Open Registry Editor

Press:

```text
Win + R
```

Enter:

```text
regedit
```

Press **Enter**.

Approve the User Account Control prompt if Windows displays one.

## 2. Verify the `.txt` Preview Handler

Navigate to:

```text
HKEY_CLASSES_ROOT\.txt\shellex\{8895b1c6-b41f-4c1c-a562-0d564250836f}
```

The `(Default)` value should be:

```text
{D8034CFA-F34B-41FE-AD45-62FCBB52A6DA}
```

This is the preview-handler registration that will be copied to the custom extension.

## 3. Locate or Create the Custom Extension Key

Navigate to:

```text
HKEY_CLASSES_ROOT
```

Look for:

```text
.<extension>
```

If the key already exists, leave its existing values and subkeys unchanged.

If it does not exist:

1. Right-click **HKEY_CLASSES_ROOT**.
2. Select **New → Key**.
3. Name the key:

```text
.<extension>
```

## 4. Create the `shellex` Key

Right-click:

```text
.<extension>
```

Select:

```text
New → Key
```

Name it:

```text
shellex
```

If `shellex` already exists, use the existing key.

## 5. Create the Preview Handler Key

Right-click:

```text
shellex
```

Select:

```text
New → Key
```

Name the new key exactly:

```text
{8895b1c6-b41f-4c1c-a562-0d564250836f}
```

The resulting path should be:

```text
HKEY_CLASSES_ROOT\.<extension>\shellex\{8895b1c6-b41f-4c1c-a562-0d564250836f}
```

## 6. Set the Preview Handler

Select:

```text
{8895b1c6-b41f-4c1c-a562-0d564250836f}
```

In the right pane:

1. Double-click **(Default)**.
2. Enter:

```text
{D8034CFA-F34B-41FE-AD45-62FCBB52A6DA}
```

3. Click **OK**.

The finished registry structure should look like this:

```text
HKEY_CLASSES_ROOT
└── .<extension>
    └── shellex
        └── {8895b1c6-b41f-4c1c-a562-0d564250836f}
            (Default) = {D8034CFA-F34B-41FE-AD45-62FCBB52A6DA}
```

## 7. Restart Windows Explorer

Open **Task Manager**.

Find:

```text
Windows Explorer
```

Right-click it and select:

```text
Restart
```

Signing out or restarting Windows also works.

## 8. Test the Extension

Open File Explorer and enable:

```text
View → Show → Preview pane
```

Select a file using the custom extension.

For example:

```text
example.<extension>
```

Its plain-text contents should now appear in the Preview pane.

## Important

If the custom extension already has a key under:

```text
HKEY_CLASSES_ROOT
```

do not delete or replace it.

Only add the following subtree:

```text
shellex
└── {8895b1c6-b41f-4c1c-a562-0d564250836f}
```

with:

```text
(Default) = {D8034CFA-F34B-41FE-AD45-62FCBB52A6DA}
```

This preserves any other file association or metadata already registered for the extension.