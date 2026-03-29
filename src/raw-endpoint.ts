import sharp from "sharp";
import { renderEinkImage } from "./render";
import type { SceneData } from "./data";

/**
 * Convert a 480x800 PNG to 1-bit packed framebuffer for the X4.
 * 
 * The X4's native display is 800x480 (landscape), but held portrait.
 * The framebuffer is 800 wide × 480 tall, packed 8 pixels per byte.
 * So: 800/8 * 480 = 48000 bytes.
 * 
 * Our Satori image is 480x800 (portrait). We need to:
 * 1. Convert to grayscale
 * 2. Rotate 90° clockwise to get 800x480
 * 3. Apply Floyd-Steinberg dithering to 1-bit
 * 4. Pack into bytes (MSB first)
 */
export async function renderRawFramebuffer(data: SceneData): Promise<Buffer> {
  const png = await renderEinkImage(data);
  
  // Convert to grayscale 800x480 (rotate 90° CW from 480x800)
  const { data: pixels, info } = await sharp(png)
    .rotate(90)
    .grayscale()
    .raw()
    .toBuffer({ resolveWithObject: true });
  
  const w = info.width;  // 800
  const h = info.height; // 480
  
  // Floyd-Steinberg dithering
  const gray = new Float32Array(pixels.length);
  for (let i = 0; i < pixels.length; i++) gray[i] = pixels[i];
  
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const idx = y * w + x;
      const old = gray[idx];
      const val = old > 128 ? 255 : 0;
      gray[idx] = val;
      const err = old - val;
      
      if (x + 1 < w)               gray[idx + 1]     += err * 7 / 16;
      if (y + 1 < h && x - 1 >= 0) gray[(y+1)*w+x-1] += err * 3 / 16;
      if (y + 1 < h)               gray[(y+1)*w+x]    += err * 5 / 16;
      if (y + 1 < h && x + 1 < w)  gray[(y+1)*w+x+1]  += err * 1 / 16;
    }
  }
  
  // Pack to 1-bit (MSB first, white=1, black=0)
  const packedSize = (w / 8) * h; // 100 * 480 = 48000
  const packed = Buffer.alloc(packedSize);
  
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const byteIdx = y * (w / 8) + Math.floor(x / 8);
      const bitIdx = 7 - (x % 8); // MSB first
      if (gray[y * w + x] > 128) {
        packed[byteIdx] |= (1 << bitIdx); // white
      }
      // black is already 0
    }
  }
  
  return packed;
}
