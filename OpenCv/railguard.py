"""
RailGuard AI - live crack detection + event logging (single file)
Run:   python3 railguard.py
Quit:  press q (or Ctrl+C if no display)
Test without camera:  python3 railguard.py track_crack.png
"""
import cv2, json, os, sys, time, numpy as np
from datetime import datetime
from pathlib import Path

# ---------------- settings ----------------
TRIP_ID    = "TRIP-001"
SEGMENT_ID = "SIM-TRACK-01"
LATITUDE, LONGITUDE = 17.0000, 78.0000     # simulated location
COOLDOWN   = 3.0                            # seconds between saved detections
OUT_DIR    = Path("demo_events")
SHOW       = bool(os.environ.get("DISPLAY")) # auto: no monitor -> no window
# ------------------------------------------

def find_rails(gray):
    H, W = gray.shape
    thr = max(90, int(np.percentile(gray, 80)))
    bright = (gray > thr).astype(np.uint8) * 255
    opened = cv2.morphologyEx(bright, cv2.MORPH_OPEN,
             cv2.getStructuringElement(cv2.MORPH_RECT, (1, max(15, H // 8))))
    joined = cv2.morphologyEx(opened, cv2.MORPH_CLOSE,
             cv2.getStructuringElement(cv2.MORPH_RECT, (1, max(25, H // 6))))
    n, lab, stats, _ = cv2.connectedComponentsWithStats(joined)
    region = np.zeros_like(joined)
    for i in range(1, n):
        x, y, w, h, a = stats[i]
        if h >= 0.6 * H and w <= 0.2 * W:
            region[lab == i] = 255
    return region, cv2.bitwise_and(opened, region)

def detect_cracks(frame):
    gray = cv2.GaussianBlur(cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY), (5, 5), 0)
    H, W = gray.shape
    scale = (H * W) / (720 * 1280)
    min_area, max_area = max(15, int(60 * scale)), max(200, int(4000 * scale))
    region, rails = find_rails(gray)
    if rails.sum() == 0:
        return []
    k = max(9, W // 60) | 1
    span = cv2.morphologyEx(region, cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_RECT, (k, k)))
    gaps = cv2.subtract(span, rails)
    rail_level = float(np.median(gray[rails > 0]))
    gaps = cv2.bitwise_and(gaps, (gray < rail_level * 0.62).astype(np.uint8) * 255)
    gaps = cv2.morphologyEx(gaps, cv2.MORPH_OPEN, np.ones((2, 2), np.uint8))
    gaps = cv2.dilate(gaps, np.ones((5, 5), np.uint8))
    found = []
    for c in cv2.findContours(gaps, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)[0]:
        if not (min_area <= cv2.contourArea(c) <= max_area):
            continue
        x, y, w, h = cv2.boundingRect(c)
        m = 6
        if y <= m or y + h >= H - m or x <= m or x + w >= W - m:
            continue
        score = round(min(1.0, (rail_level - float(gray[y:y+h, x:x+w].min())) / 255.0 * 1.2), 2)
        found.append((x, y, w, h, score))
    return found

def save_event(original, annotated, box, score, count):
    OUT_DIR.mkdir(exist_ok=True)
    now = datetime.now()
    event_id = f"RG-{now:%Y%m%d-%H%M%S}"
    img_path = OUT_DIR / f"{event_id}.jpg"
    cv2.imwrite(str(img_path), original)                       # original evidence
    cv2.imwrite(str(OUT_DIR / f"{event_id}_annotated.jpg"), annotated)
    x, y, w, h = box
    event = {
        "event_id": event_id,
        "trip_id": TRIP_ID,
        "timestamp": now.isoformat(timespec="seconds"),
        "segment_id": SEGMENT_ID,
        "location": {"latitude": LATITUDE, "longitude": LONGITUDE, "source": "simulated"},
        "defect_type": "visible_crack_like_mark",
        "detection_score": score,
        "bounding_box": {"x": x, "y": y, "w": w, "h": h},
        "detection_method": "opencv_controlled_detector",
        "image": str(img_path),
        "status": "pending_human_review",
    }
    with open(OUT_DIR / "events.jsonl", "a") as f:             # one line per event
        f.write(json.dumps(event) + "\n")
    return event

def main():
    test_img = sys.argv[1] if len(sys.argv) > 1 else None
    if test_img:
        still = cv2.imread(test_img)
        cap = None
    else:
        cap = cv2.VideoCapture(0)
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        if not cap.isOpened():
            print("Camera NOT opened. Try VideoCapture(1).")
            return
    print("RailGuard running... watching the track.")
    last_save, n_events, frames = 0, 0, 0
    while True:
        if cap:
            ok, frame = cap.read()
            if not ok:
                break
        else:
            frame = still.copy()
            time.sleep(0.05)
        frames += 1
        hits = detect_cracks(frame)
        shown = frame.copy()
        for (x, y, w, h, s) in hits:
            cv2.rectangle(shown, (x, y), (x + w, y + h), (0, 0, 255), 2)
            cv2.putText(shown, f"CRACK DETECTED {s}", (x, max(y - 8, 14)),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)
        if hits and time.time() - last_save > COOLDOWN:
            best = max(hits, key=lambda h: h[4])
            ev = save_event(frame, shown, best[:4], best[4], n_events)
            n_events += 1
            last_save = time.time()
            print(f"CRACK DETECTED | {ev['event_id']} | score {ev['detection_score']} "
                  f"| {ev['segment_id']} | saved {ev['image']}")
        if SHOW:
            cv2.imshow("RailGuard", shown)
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break
        if test_img and frames >= 40:
            break
    if cap:
        cap.release()
    cv2.destroyAllWindows()
    print(f"Stopped. Events saved: {n_events}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")

