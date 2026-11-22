import json, time
from pathlib import Path
from hindsight_client import Hindsight

client = Hindsight(
    base_url="https://api.hindsight.vectorize.io",
    api_key="hsk_1c4e5b841a6505c140085d3e7aabe806_b685adfe3f4077f8",
)
BANK = "railguard-test"          # same bank as hs_test.py

# Newest event from the Pi (copy demo_events/events.jsonl next to this file).
# If you haven't copied it yet, a sample event is used and labeled as SAMPLE.
p = Path("demo_events/events.jsonl")
if p.exists():
    ev = json.loads([l for l in p.read_text().splitlines() if l.strip()][-1])
    src = "REAL event from the robot"
else:
    ev = {"event_id": "RG-SAMPLE", "trip_id": "TRIP-001", "segment_id": "SIM-TRACK-01",
          "defect_type": "visible_crack_like_mark", "detection_score": 0.91}
    src = "SAMPLE event (not from the robot)"

print(f"=== NEW EVENT ({src}) ===")
print(ev["event_id"], ev["trip_id"], ev["segment_id"], "score", ev["detection_score"])

print("\n--- WITHOUT MEMORY ---")
print(f"{ev['defect_type']} detected at {ev['segment_id']} (score {ev['detection_score']}). "
      "No previous context. Human verification recommended.")

print("\n--- WITH HINDSIGHT MEMORY (real recall) ---")
res = client.recall(bank_id=BANK,
      query=f"What was previously observed at {ev['segment_id']} on earlier trips like TRIP-000? {ev['defect_type']}")
mems = [r.text for r in res.results][:6]
if mems:
    print("Hindsight recalled:")
    for m in mems:
        print(" -", m)
    print(f"\nAssessment: a similar finding exists in memory for {ev['segment_id']}. "
          f"Today's score is {ev['detection_score']}. Scores alone do not show growth "
          "or severity. Compare the original images and request human verification.")
else:
    print("Nothing recalled yet. Wait 30 seconds and run again.")

# The new event is retained too, so the next inspection can recall it
client.retain(
    bank_id=BANK,
    content=f"{ev['trip_id']} at segment {ev['segment_id']}: {ev['defect_type']}, "
            f"detector score {ev['detection_score']}, review status pending.",
)
print("\nNew event saved to Hindsight memory for future inspections.")