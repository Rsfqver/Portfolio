import cv2
import socket
import struct
from datetime import datetime, timedelta
from ultralytics import YOLO
from gpiozero import PWMOutputDevice
import time



# Set up the camera with Picam
# picam2 = Picamera2()
# picam2.preview_configuration.main.size = (1280, 1280)
# picam2.preview_configuration.main.format = "RGB888"
# picam2.preview_configuration.align()
# picam2.configure("preview")
# picam2.start()

# Load YOLOv8
model = YOLO("weapon.pt")

# Set up socket connection
# server_ip = '192.168.0.140' #TODO : set ip
# server_port = 8090 #TODO : set port num

# client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# client_socket.connect((server_ip, server_port))

last_sent_time = datetime.now() - timedelta(seconds=10)  # Initialize to allow the first send

rtsp_url = "rtsp://127.0.0.1:8554/test"
cap = cv2.VideoCapture(rtsp_url)
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
frame_skip = 15
frame_count = 0

# Define the speaker pin
SPKR = 14  # GPIO 14
buzzer = PWMOutputDevice(SPKR)
   # 부저 울리는 함수
def trigger_buzzer():
        buzzer.on()  # 부저 ON
        time.sleep(0.1)  # 0.1초 동안 삑
        buzzer.off()   # 부저 OFF
        time.sleep(0.1)  # 0.1초 대기
     
        buzzer.on()  # 부저 ON
        time.sleep(0.1)  # 0.1초 동안 삑
        buzzer.off()   # 부저 OFF
 


while True:
    frame_count += 1
    # print("Start Code")
    # Capture a frame from the camera
    # frame = picam2.capture_array()
    ret, frame = cap.read()
    if frame_count % frame_skip != 0:
        continue
    frame = cv2.resize(frame, None, fx=0.5, fy=0.5)
    if not ret:
        print("Error: Can't receive frame (stream end?). Exiting ...")
        break
    
    # Run YOLO model on the captured frame and store the results
    results = model.track(frame, persist=True, verbose=True)
    detected_objects = []
    
    if results[0].boxes:  # Check if there are any boxes detected
        boxes = results[0].boxes.xyxy.cpu()
        class_ids = results[0].boxes.cls.cpu().tolist()
        confidences = results[0].boxes.conf.cpu().tolist()
        names = model.names

        for box, class_id, confidence in zip(boxes, class_ids, confidences):
            x1, y1, x2, y2 = map(int, box)
            if confidence >= 0.5:
                trigger_buzzer()
                detected_object = names[int(class_id)]
                detected_objects.append(detected_object)

                label = f"{detected_object} {confidence:.2f} "

                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
                label_size, base_line = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
                # y1 = max(y1, label_size[1] + 100)
                cv2.rectangle(frame, (x1, y1 - label_size[1] - 10), (x1 + label_size[0], y1 + base_line - 10),
                                (0, 255, 0), cv2.FILLED)
                cv2.putText(frame, label, (x1, y1 - 7), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)

                cv2.putText(frame, label, (x1, y1 - 7), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)

                text = "BODY CAM"

                cv2.putText(frame, text, (500, 60), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 3)
                
                # Send image if detection occurred and 10 seconds have passed
                if detected_objects and (datetime.now() - last_sent_time).total_seconds() >= 10:
                    
                    detected_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S").encode('utf-8')
                    
                    _, buffer = cv2.imencode('.jpg', frame)
                    frame_bytes = buffer.tobytes()
                    
                    # # Send the detected time size and data
                    # client_socket.sendall(len(detected_time).to_bytes(4, 'big'))
                    # client_socket.sendall(detected_time)

                    # # Send the file size (4 bytes)
                    # client_socket.sendall(len(frame_bytes).to_bytes(4, 'big'))

                    # # Send the file data
                    # client_socket.sendall(frame_bytes)

                    last_sent_time = datetime.now()  # Update the last sent time
                    print(f"Sent image at {last_sent_time}")
                

            # Show the processed video
    cv2.imshow("Weapon Detection Output", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):  # Press 'q' to quit
        break


# Close all windows
# client_socket.close()
cv2.destroyAllWindows()

buzzer.close()
