# Fix WSL Networking for Obsentra Server

## Problem
- Server runs on `192.168.0.108:3000` in WSL
- Can access via `localhost:3000` from Windows
- Cannot access via IP `192.168.0.108:3000` from Windows browsers
- ESP32 needs to connect via IP, not localhost

## Solution 1: Windows Firewall Fix (Try this first)

**Run these commands in Windows PowerShell as Administrator:**

```powershell
# Allow Node.js through Windows Firewall
netsh advfirewall firewall add rule name="WSL Obsentra Server" dir=in action=allow protocol=TCP localport=3000 profile=any

# If above doesn't work, try this more specific rule
netsh advfirewall firewall add rule name="WSL Obsentra Inbound" dir=in action=allow protocol=TCP localport=3000 remoteip=192.168.0.0/24 profile=any

# Check if the rule was added
netsh advfirewall firewall show rule name="WSL Obsentra Server"
```

## Solution 2: WSL Configuration Update

**Create/update WSL config file:**

1. Create `.wslconfig` file in Windows user directory (`C:\Users\{username}\.wslconfig`):

```ini
[wsl2]
networkingMode=mirrored
dnsTunneling=true
firewall=true
autoProxy=true
```

2. Restart WSL in Windows PowerShell (as Administrator):
```powershell
wsl --shutdown
wsl
```

## Solution 3: Port Forwarding (If above fails)

**In Windows PowerShell as Administrator:**

```powershell
# Forward port 3000 from Windows to WSL
netsh interface portproxy add v4tov4 listenport=3000 listenaddress=0.0.0.0 connectport=3000 connectaddress=192.168.0.108

# Check if forwarding is active
netsh interface portproxy show all

# To remove later (if needed):
# netsh interface portproxy delete v4tov4 listenport=3000 listenaddress=0.0.0.0
```

## Test Steps

1. After applying any solution, test in Windows browser:
   - `http://192.168.0.108:3000`
   - Should show: `{"message":"Obsentra Core Server","version":"1.0.0"...}`

2. If working, update ESP32 to use the IP address `192.168.0.108`

## Alternative: Use localhost in ESP32

If IP access still doesn't work, configure ESP32 to use:
- Server field: `localhost` (instead of `192.168.0.108`)
- This works because WSL mirrored networking allows ESP32 to reach localhost
