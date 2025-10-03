import serial
import pygame
import numpy as np

WIDTH = 64
HEIGHT = 64
FRAME_SIZE_BYTES = WIDTH * HEIGHT // 4
SCALE = 8
FRAME_SYNC = b'&'

# Open serial port (adjust COM port)
ser = serial.Serial("COM6", 921600, timeout=0.01)

def decompress_4pixels(data):
    buf = bytearray()
    for b in data:
        buf.extend([(b >> 6) & 0x03,
                    (b >> 4) & 0x03,
                    (b >> 2) & 0x03,
                    b & 0x03])
    return buf

def scale_to_rgb(frame):
    gray8 = np.array(frame, dtype=np.uint8) * 85
    rgb = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)
    rgb[:, :, 0] = gray8.reshape((HEIGHT, WIDTH))
    rgb[:, :, 1] = rgb[:, :, 0]
    rgb[:, :, 2] = rgb[:, :, 0]
    return rgb

def read_frame():
    # Wait for sync marker
    while True:
        b = ser.read(1)
        if b == FRAME_SYNC:
            break
    raw = ser.read(FRAME_SIZE_BYTES)
    while len(raw) < FRAME_SIZE_BYTES:
        raw += ser.read(FRAME_SIZE_BYTES - len(raw))
    return raw

def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH*SCALE, HEIGHT*SCALE))
    pygame.display.set_caption("Framebuffer Viewer")
    clock = pygame.time.Clock()
    surf = pygame.Surface((WIDTH, HEIGHT))

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                ser.close()
                return

        raw = read_frame()
        frame = decompress_4pixels(raw)
        rgb_array = scale_to_rgb(frame)

        pygame.surfarray.blit_array(surf, rgb_array)
        screen.blit(pygame.transform.scale(surf, (WIDTH*SCALE, HEIGHT*SCALE)), (0, 0))
        pygame.display.flip()
        clock.tick(24)  # match MCU FPS

if __name__ == "__main__":
    main()
