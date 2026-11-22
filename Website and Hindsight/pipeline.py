import json, shutil
from pathlib import Path
from datetime import datetime
from hindsight_client import Hindsight

client = Hindsight(base_url="https://api.hindsight.vectorize.io", api_key="hsk_1c4e5b841a6505c140085d3e7aabe806_b685adfe3f4077f8")
BANK = "railguard-test"

SRC = Path("demo_events")
OUT = Path("dashboard_data")
(OUT / "images").mkdir(parents=True, exist_ok=True)

events = [json.loads(l) for l in (SRC / "events.jsonl").read_text().splitlines() if l.strip()]
print(f"Found {len(events)} events")

records = []
for ev in events:
    eid = ev["event_id"]

    # copy original + annotated photo next to the data file
    imgs = {}
    for kind, name in (("original", f"{eid}.jpg"), ("annotated", f"{eid}_annotated.jpg")):
        if (SRC / name).exists():
            shutil.copy(SRC / name, OUT / "images" / name)
            imgs[kind] = f"images/{name}"

    # real Hindsight recall
    query = (f"What was previously observed at {ev['segment_id']} on earlier trips "
             f"like TRIP-000? {ev['defect_type']}")
    res = client.recall(bank_id=BANK, query=query)
    recalled = [r.text for r in res.results][:6]

    score = ev.get("detection_score")
    if recalled:
        assessment = "recurring_or_unverified"
        explanation = (f"Similar findings exist in memory for {ev['segment_id']}. "
                       f"Current detector score is {score}. Scores alone do not show growth "
                       "or severity. Compare original images and request human verification.")
        action = "Compare images and request human verification"
    else:
        assessment = "new"
        explanation = "No prior memory for this segment."
        action = "Request human verification"

    records.append({
        "event": ev,
        "images": imgs,
        "location_note": "Simulated location",
        "memory_used": bool(recalled),
        "recalled_memories": recalled,
        "assessment": assessment,
        "explanation": explanation,
        "suggested_action": action,
        "review_status": ev.get("status", "pending_human_review"),
        "generated_at": datetime.now().isoformat(timespec="seconds"),
    })
    print("processed", eid)

records.sort(key=lambda r: r["event"]["event_id"], reverse=True)
(OUT / "data.json").write_text(json.dumps(records, indent=2))
print(f"\nSaved dashboard_data/data.json with {len(records)} records")