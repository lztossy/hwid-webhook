sends hardware serials to a webhook when program is ran. 
# instructions
1. make sure to add `winhttp.lib` in properties > linker > input
2. replace webhook URL with yours
   
sends serial such as but not limited to:
- disk
- cpu
- bios
- baseboard
- smbios uuid
- mac address

for ease of access, you can add identifiers which you want sent via batch to the code. 
