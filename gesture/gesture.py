import cv2
import mediapipe as mp
import numpy as np
import socket
from collections import deque
import platform
import time

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

pitch_buffer = deque(maxlen=15)
roll_buffer  = deque(maxlen=15)

mp_hands = mp.solutions.hands
hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    model_complexity=1,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)
mp_draw = mp.solutions.drawing_utils

last_sent_pitch = None
last_sent_roll  = None
THRESHOLD       = 1

MAX_OFFSET_X      = 200
MAX_OFFSET_Y      = 150
CENTER_THRESHOLD  = 50
COUNTDOWN_SECONDS = 60

def to_pixel_coords(landmark, image_shape):
    h, w, _ = image_shape
    return np.array([landmark.x * w, landmark.y * h, landmark.z * w])

def weighted_avg(buf):
    weights = list(range(1, len(buf) + 1))
    return int(sum(v * w for v, w in zip(buf, weights)) / sum(weights))

def do_full_reset():
    """Send flat, clear all buffers, return values to set state=waiting."""
    sock.sendto("A0,0*".encode(), (UDP_IP, UDP_PORT))
    pitch_buffer.clear()
    roll_buffer.clear()
    print("Maze reset — waiting for hand to center.")

def draw_timer(frame, seconds_left, cx):
    radius = 55
    color  = (0, 200, 0) if seconds_left > 10 else (0, 60, 255)
    cv2.circle(frame, (cx, 60), radius, (30, 30, 30), -1)
    cv2.circle(frame, (cx, 60), radius, color, 3)
    timer_str  = str(int(seconds_left))
    font_scale = 1.4 if seconds_left >= 10 else 1.8
    text_size  = cv2.getTextSize(timer_str, cv2.FONT_HERSHEY_SIMPLEX, font_scale, 3)[0]
    cv2.putText(frame, timer_str,
                (cx - text_size[0] // 2, 60 + text_size[1] // 2),
                cv2.FONT_HERSHEY_SIMPLEX, font_scale, color, 3)
    cv2.putText(frame, "TIME LEFT", (cx - 45, 125),
                cv2.FONT_HERSHEY_SIMPLEX, 0.45, (180, 180, 180), 1)

cap = cv2.VideoCapture(0, cv2.CAP_DSHOW) if platform.system() == "Windows" else cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH,  1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

cv2.namedWindow("Palm Tilt Tracker", cv2.WND_PROP_FULLSCREEN)
cv2.setWindowProperty("Palm Tilt Tracker", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

state            = "waiting"   # "waiting" | "tracking"
show_reset_flash = 0
timer_start      = None

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    frame = cv2.flip(frame, 1)

    screen_w = cv2.getWindowImageRect("Palm Tilt Tracker")[2]
    screen_h = cv2.getWindowImageRect("Palm Tilt Tracker")[3]
    if screen_w > 0 and screen_h > 0:
        frame = cv2.resize(frame, (screen_w, screen_h))

    rgb     = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(rgb)

    h, w, _ = frame.shape
    center_x, center_y = w // 2, h // 2
    box_size = 100

    cv2.drawMarker(frame, (center_x, center_y), (200, 200, 200),
                   markerType=cv2.MARKER_CROSS, markerSize=20, thickness=2)
    cv2.rectangle(frame,
                  (center_x - box_size, center_y - box_size),
                  (center_x + box_size, center_y + box_size),
                  (100, 100, 100), 1)

    # ── Timer logic — only runs in tracking state ─────────────
    if state == "tracking" and timer_start is not None:
        elapsed      = time.time() - timer_start
        seconds_left = max(0.0, COUNTDOWN_SECONDS - elapsed)
        draw_timer(frame, seconds_left, center_x)

        if seconds_left <= 0:
            do_full_reset()
            state            = "waiting"   # ← back to waiting, hand must re-center
            timer_start      = None
            last_sent_pitch  = None
            last_sent_roll   = None
            show_reset_flash = 60
            print("Timer hit 0 — returning to waiting state.")
    # ──────────────────────────────────────────────────────────

    if state == "waiting":
        # Still draw landmarks so user can see their hand
        if results.multi_hand_landmarks and results.multi_handedness:
            for hand_landmarks, handedness in zip(results.multi_hand_landmarks, results.multi_handedness):
                mp_draw.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)
                knuckles = [to_pixel_coords(hand_landmarks.landmark[i], frame.shape) for i in [5, 9, 13, 17]]
                knuckle_center = np.mean(knuckles, axis=0)
                hand_x, hand_y = int(knuckle_center[0]), int(knuckle_center[1])
                cv2.circle(frame, (hand_x, hand_y), 8, (0, 255, 255), -1)

                dist_from_center = np.sqrt((hand_x - center_x)**2 + (hand_y - center_y)**2)

                if dist_from_center < CENTER_THRESHOLD:
                    state            = "tracking"
                    timer_start      = time.time()
                    last_sent_pitch  = None
                    last_sent_roll   = None
                    pitch_buffer.clear()
                    roll_buffer.clear()
                    print("Hand centered — tracking + timer started!")

        cv2.putText(frame, "Move hand to center to start",
                    (center_x - 160, center_y + box_size + 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (180, 180, 180), 2)
        cv2.putText(frame, "STATE: WAITING FOR CENTER  |  R = Reset  |  Q = Quit",
                    (10, h - 20), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 100, 255), 2)

    elif state == "tracking":
        if results.multi_hand_landmarks and results.multi_handedness:
            for hand_landmarks, handedness in zip(results.multi_hand_landmarks, results.multi_handedness):
                mp_draw.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

                hand_label = handedness.classification[0].label
                cv2.putText(frame, f"Hand: {hand_label}", (10, 300),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (200, 200, 0), 2)

                knuckles = [to_pixel_coords(hand_landmarks.landmark[i], frame.shape) for i in [5, 9, 13, 17]]
                knuckle_center = np.mean(knuckles, axis=0)
                hand_x, hand_y = int(knuckle_center[0]), int(knuckle_center[1])
                cv2.circle(frame, (hand_x, hand_y), 8, (0, 255, 255), -1)

                offset_x = hand_x - center_x
                offset_y = center_y - hand_y

                rel_roll  = np.clip((offset_x  / MAX_OFFSET_X) * 90, -90, 90)
                rel_pitch = np.clip((-offset_y / MAX_OFFSET_Y) * 90, -90, 90)

                mapped_pitch  = 90 + rel_pitch
                mapped_roll   = 90 + rel_roll
                arduino_pitch = int(mapped_pitch - 90)
                arduino_roll  = int(mapped_roll  - 90)

                pitch_buffer.append(arduino_pitch)
                roll_buffer.append(arduino_roll)

                smoothed_pitch = weighted_avg(pitch_buffer)
                smoothed_roll  = weighted_avg(roll_buffer)

                pitch_changed = last_sent_pitch is None or abs(smoothed_pitch - last_sent_pitch) > THRESHOLD
                roll_changed  = last_sent_roll  is None or abs(smoothed_roll  - last_sent_roll)  > THRESHOLD

                if pitch_changed or roll_changed:
                    sock.sendto(f"A{smoothed_pitch},{smoothed_roll}*".encode(), (UDP_IP, UDP_PORT))
                    last_sent_pitch = smoothed_pitch
                    last_sent_roll  = smoothed_roll
                    cv2.putText(frame, f"Sent Pitch: {smoothed_pitch}, Roll: {smoothed_roll}",
                                (10, 270), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
                else:
                    cv2.putText(frame, "No change (threshold)",
                                (10, 270), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (100, 100, 100), 2)

                cv2.putText(frame, f"Offset X: {offset_x:.1f}",      (10, 30),  cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)
                cv2.putText(frame, f"Offset Y: {offset_y:.1f}",      (10, 60),  cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                cv2.putText(frame, f"Pitch: {rel_pitch:.2f}",        (10, 90),  cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                cv2.putText(frame, f"Roll:  {rel_roll:.2f}",         (10, 120), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)
                cv2.putText(frame, f"Mapped Pitch: {mapped_pitch:.2f}", (10, 150), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (100, 255, 100), 2)
                cv2.putText(frame, f"Mapped Roll:  {mapped_roll:.2f}", (10, 180), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (100, 255, 100), 2)

        else:
            # Hand lost mid-tracking → full reset to waiting
            do_full_reset()
            state            = "waiting"
            timer_start      = None
            last_sent_pitch  = None
            last_sent_roll   = None
            show_reset_flash = 45
            print("Hand lost — resetting and returning to waiting.")

        cv2.putText(frame, "STATE: TRACKING  |  R = Reset  |  Q = Quit",
                    (10, h - 20), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

    # Reset flash overlay
    if show_reset_flash > 0:
        cv2.putText(frame, "MAZE RESET!", (center_x - 120, center_y - 140),
                    cv2.FONT_HERSHEY_SIMPLEX, 1.6, (0, 80, 255), 4)
        show_reset_flash -= 1

    cv2.imshow("Palm Tilt Tracker", frame)
    key = cv2.waitKey(1) & 0xFF

    if key == ord('q'):
        break
    elif key == ord('r'):
        do_full_reset()
        state            = "waiting"   # ← must re-center hand to resume
        timer_start      = None
        last_sent_pitch  = None
        last_sent_roll   = None
        show_reset_flash = 45
        print("Manual reset — waiting for hand to center.")

cap.release()
cv2.destroyAllWindows()