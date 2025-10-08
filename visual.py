#!/usr/bin/env python3
from pyvis.network import Network
import re

TRACE = """
in 0x5d0f16515190 
made 0x5d0f1653ba10 0x5d0f1653bac0 0x5d0f1653bb40
setting insertion at 0x5d0f1653ba10 
in 0x5d0f1653ba10 
made 0x5d0f1653c090 0x5d0f1653c110 0x5d0f1653c190
setting insertion at 0x5d0f1653c090 
in 0x5d0f1653c090 
setting insertion at 0x5d0f1653c110 
adding merge in 0x5d0f1653c110 to 0x5d0f1653c190
in 0x5d0f1653c110 
setting insertion at 0x5d0f1653c190 
in 0x5d0f1653c190 
setting insertion at 0x5d0f1653bac0 
adding merge in 0x5d0f1653bac0 to 0x5d0f1653bb40
in 0x5d0f1653bac0 
setting insertion at 0x5d0f1653bb40 
""".strip().splitlines()

# ------------------------------------------------------------
# Parse events
# ------------------------------------------------------------
events = []
for line in TRACE:
    if m := re.match(r"in (0x[0-9a-f]+)", line):
        events.append(("in", m.group(1)))
    elif m := re.match(r"made (0x[0-9a-f]+(?: 0x[0-9a-f]+)*)", line):
        events.append(("made", m.group(1).split()))
    elif m := re.match(r"setting insertion at (0x[0-9a-f]+)", line):
        events.append(("set", m.group(1)))
    elif m := re.match(r"adding merge in (0x[0-9a-f]+) to (0x[0-9a-f]+)", line):
        events.append(("merge", (m.group(1), m.group(2))))

# ------------------------------------------------------------
# Graph building
# ------------------------------------------------------------
net = Network(height="900px", width="100%", directed=True, bgcolor="#222", font_color="white")

net.set_options("""
{
  "physics": { "enabled": false },
  "edges": {
    "smooth": false,
    "arrows": { "to": { "enabled": true } },
    "color": { "inherit": false }
  },
  "nodes": {
    "shape": "dot",
    "size": 14,
    "font": { "color": "white", "size": 16 },
    "borderWidth": 2
  }
}
""")

colors = {"made": "blue", "set": "green", "merge": "red", "in": "gray"}
added = set()
last_in = None  # Track last "in" explicitly
last_node = None

# find the root (first in)
root_addr = None
for etype, data in events:
    if etype == "in":
        root_addr = data
        net.add_node(root_addr, label=root_addr, color=colors["in"], x=0, y=-400, physics=False)
        added.add(root_addr)
        last_in = root_addr
        break

# now parse everything
for etype, data in events:
    if etype == "in":
        addr = data
        if addr == root_addr:
            continue
        if addr not in added:
            net.add_node(addr, label=addr, color=colors["in"])
            added.add(addr)
        last_in = addr
        last_node = addr

    elif etype == "made":
        parent = last_in  # ✅ fix: made belongs to the last "in"
        for addr in data:
            if addr not in added:
                net.add_node(addr, label=addr, color=colors["made"])
                added.add(addr)
            if parent:
                net.add_edge(parent, addr, color=colors["made"])
        last_node = data[-1]

    elif etype == "set":
        last_node = data  # no implicit edge

    elif etype == "merge":
        src, dst = data
        for addr in (src, dst):
            if addr not in added:
                net.add_node(addr, label=addr, color=colors["merge"])
                added.add(addr)
        net.add_edge(src, dst, color=colors["merge"], width=3)  # thicker for merges

# ------------------------------------------------------------
net.show("builder_trace.html")
print("✅ Open builder_trace.html in your browser — corrected parent logic for 'made'.")
