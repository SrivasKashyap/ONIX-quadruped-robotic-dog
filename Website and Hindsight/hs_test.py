import time
from hindsight_client import Hindsight

client = Hindsight(
    base_url="https://api.hindsight.vectorize.io",
    api_key="hsk_1c4e5b841a6505c140085d3e7aabe806_b685adfe3f4077f8",
)

client.retain(
    bank_id="railguard-test",
    content="TRIP-000 at segment SIM-TRACK-01: possible crack-like mark, "
            "detector score 0.74, review status inconclusive.",
)
print("saved the memory, waiting...")
time.sleep(20)

res = client.recall(bank_id="railguard-test",
                    query="What was seen at SIM-TRACK-01 before?")
for r in res.results:
    print("-", r.text)