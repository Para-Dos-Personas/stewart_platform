import cv2
import mediapipe as mp
import numpy as np
import socket
from collections import deque
import platform

UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

pitch_buffer = deque(maxlen=15)
roll_buffer = deque(maxlen=15)

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
last_sent_roll = None
THRESHOLD = 1

def to_pixel_coords(landmark, image_shape):
    h, w, _ = image_shape
    return np.array([landmark.x * w, landmark.y * h, landmark.z * w])

def vector_to_angles(normal):
    pitch = np.arcsin(-normal[1])
    roll = np.arctan2(normal[0], normal[2])
    yaw = np.arctan2(normal[0], normal[1])
    return np.degrees([pitch, roll, yaw])

def weighted_avg(buf):
    weights = list(range(1, len(buf) + 1))
    return int(sum(v * w for v, w in zip(buf, weights)) / sum(weights))

cap = cv2.VideoCapture(0, cv2.CAP_DSHOW) if platform.system() == "Windows" else cv2.VideoCapture(0)

zero_pitch, zero_roll, zero_yaw = None, None, None
pitch, roll, yaw = 0, 0, 0

while True:
    ret, frame = cap.read()
    if not ret:
        print("Failed to grab frame")
        break

    frame = cv2.flip(frame, 1)
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(rgb)

    if results.multi_hand_landmarks and results.multi_handedness:
        for hand_landmarks, handedness in zip(results.multi_hand_landmarks, results.multi_handedness):
            mp_draw.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

            # Get which hand it is
            hand_label = handedness.classification[0].label  # "Left" or "Right"
            cv2.putText(frame, f"Hand: {hand_label}", (10, 300), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (200, 200, 0), 2)

            wrist = to_pixel_coords(hand_landmarks.landmark[0], frame.shape)
            index_base = to_pixel_coords(hand_landmarks.landmark[5], frame.shape)
            pinky_base = to_pixel_coords(hand_landmarks.landmark[17], frame.shape)

            v1 = index_base - wrist
            v2 = pinky_base - wrist
            normal = np.cross(v1, v2)
            normal = normal / np.linalg.norm(normal)

            # Flip normal for left hand so angles are consistent with right hand
            if hand_label == "Left":
                normal = -normal

            pitch, roll, yaw = vector_to_angles(normal)

            cv2.putText(frame, f"Pitch: {pitch:.2f}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            cv2.putText(frame, f"Roll: {roll:.2f}", (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)
            cv2.putText(frame, f"Yaw: {yaw:.2f}", (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

            if zero_pitch is not None:
                rel_pitch = (pitch - zero_pitch) / 1.2
                rel_roll = (roll - zero_roll) / 1.2
                rel_yaw = yaw - zero_yaw

                mapped_pitch = max(0, min(180, 90 + rel_pitch))
                mapped_roll  = max(0, min(180, 90 + rel_roll))
                mapped_yaw   = max(0, min(180, 90 + rel_yaw))

                arduino_pitch = int(mapped_pitch - 90)
                arduino_roll = int(mapped_roll - 90)

                pitch_buffer.append(arduino_pitch)
                roll_buffer.append(arduino_roll)

                smoothed_pitch = weighted_avg(pitch_buffer)
                smoothed_roll = weighted_avg(roll_buffer)

                pitch_changed = last_sent_pitch is None or abs(smoothed_pitch - last_sent_pitch) > THRESHOLD
                roll_changed = last_sent_roll is None or abs(smoothed_roll - last_sent_roll) > THRESHOLD

                if pitch_changed or roll_changed:
                    message = f"A{smoothed_pitch},{smoothed_roll}*"
                    sock.sendto(message.encode(), (UDP_IP, UDP_PORT))
                    last_sent_pitch = smoothed_pitch
                    last_sent_roll = smoothed_roll
                    cv2.putText(frame, f"Sent Pitch: {smoothed_pitch}, Roll: {smoothed_roll}", (10, 270), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
                else:
                    cv2.putText(frame, f"No change (threshold)", (10, 270), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (100, 100, 100), 2)

                cv2.putText(frame, f"Pitch Delta: {rel_pitch:.2f}", (10, 120), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
                cv2.putText(frame, f"Roll Delta: {rel_roll:.2f}", (10, 150), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)
                cv2.putText(frame, f"Yaw Delta: {rel_yaw:.2f}", (10, 180), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 255), 2)
                cv2.putText(frame, f"Mapped Yaw: {mapped_yaw:.2f}", (10, 210), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (100, 255, 100), 2)
                cv2.putText(frame, f"Mapped Roll: {mapped_roll:.2f}", (10, 240), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (100, 255, 100), 2)

    if zero_pitch is None:
        h, w, _ = frame.shape
        center_x, center_y = w // 2, h // 2
        box_size = 100
        cv2.drawMarker(frame, (center_x, center_y), (200, 200, 200), markerType=cv2.MARKER_CROSS, markerSize=20, thickness=2)
        cv2.rectangle(frame, (center_x - box_size, center_y - box_size), (center_x + box_size, center_y + box_size), (100, 100, 100), 1)
        cv2.putText(frame, "Place hand in center & press 'C' to calibrate", (center_x - 180, center_y + box_size + 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (180, 180, 180), 2)

    cv2.imshow("Palm Tilt Tracker", frame)
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('c'):
        zero_pitch, zero_roll, zero_yaw = pitch, roll, yaw
        print("Calibrated!")

cap.release()
cv2.destroyAllWindows()