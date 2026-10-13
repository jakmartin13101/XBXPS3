# Running XBXPS3 on an Xbox Series X|S

## 1. Put the console in Developer Mode

1. Install **Xbox Dev Mode** from the Microsoft Store on the console and open it. It shows an activation code.
2. On a PC, go to <https://aka.ms/activatexbox>. Sign in with a Microsoft Partner Center developer account and enter the code.
   - Individual registration has been free since mid-2025, with ID verification. Before that it cost US$19.
3. Back on the console, switch to Developer Mode. It reboots into **Dev Home**.
4. In Dev Home:
   - Under **Manage dev storage**, allocate at least 5 GB. More is better: LLVM caches and installed games land there unless you use a USB drive.
   - Under **Remote access settings**, enable **Xbox Device Portal** and set a user name and password.

Keep the console in Developer Mode only while you use it. Retail games and apps cannot run in Developer Mode.

## 2. Install the package

From the PC that built XBXPS3, in PowerShell 7:

```powershell
./scripts/deploy.ps1 -Console 192.168.1.50   # the IP shown in Dev Home
```

To install by hand instead:
1. Open `https://<xbox-ip>:11443` and accept the certificate warning.
2. Under **Home → My games & apps → Add**, upload `build/app/AppPackages/.../XBXPS3_*.msix`.
3. Add the `Dependencies/x64/*.appx` files and `build/XBXPS3.cer`.

## 3. Set XBXPS3 to "Game". This step is required.

Dev Home → highlight **XBXPS3** → **View details** (Menu button) → **App type: Game**.

| App type | Memory | GPU | Result |
|---|---|---|---|
| App (default) | ~1 GB | 45 % | RPCS3 cannot even reserve its working set |
| Game | ~5 GB | 100 % | What XBXPS3 needs |

XBXPS3 checks this at startup and tells you if it is still an App.

## 4. Give it PS3 firmware and games

XBXPS3 reads from two places.

**Option A: USB drive (recommended, for space).**
1. Format the drive as **NTFS** on a PC.
2. Create `E:\XBXPS3\` (any drive letter, the app checks E: to K:).
3. Folder **Properties → Security → Edit → Add** `ALL APPLICATION PACKAGES` and give it **Full control**, applied to subfolders. Without this, the app cannot see the folder.
4. On the Xbox, set the drive up as **Media**, not "Games & apps".

```
E:\XBXPS3\
  Install\      PS3UPDAT.PUP, *.pkg, *.rap, *.edat  (imported at startup, then moved to Install\Done)
  games\        disc games: one folder per game containing PS3_GAME\, or .iso files
  dev_hdd0\     optional: create it to keep installed games and saves on the USB drive
```

**Option B: internal LocalState.** Use Device Portal → **File explorer** → `LocalAppData\XBXPS3_...\LocalState\`:

```
LocalState\
  Install\        same as above
  games\          same as above
  dev_hdd0\ dev_flash\ config\ cache\ log\   created by RPCS3
  xbxps3_probe.txt   hardware probe report, rewritten on every launch
```

On first launch with `PS3UPDAT.PUP` in an Install folder, XBXPS3 installs the firmware with an on-screen progress dialog. It then installs any `.pkg` files and reloads the game list. Get the firmware from Sony's PS3 system software page.

## 5. Logs and troubleshooting

| What | Where |
|---|---|
| Emulator log | `LocalState\log\RPCS3.log` |
| Hardware probe | `LocalState\xbxps3_probe.txt`: address space, JIT, D3D12 shader model, memory limit |
| Live warnings | Attach Visual Studio to the console (Debug → Attach to process) to see debugger output |

| Symptom | Likely cause |
|---|---|
| App closes right after the splash | A forbidden DLL import (run `scripts/check-imports.ps1` on the exe), or `vulkan_dzn.dll` is missing |
| Black screen, log says "Null renderer" | Dozen failed to load or create a device. Check the probe's shader model line |
| "Set XBXPS3 to Game mode" dialog | Step 3 |
| Games not listed | Wrong folder, or missing `ALL APPLICATION PACKAGES` permission on the USB drive |
