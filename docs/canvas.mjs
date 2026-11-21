// Use CSS pixels for geometry and scale the backing store for high-DPI screens.
export function surface(canvas) {
  const box = canvas.getBoundingClientRect();
  const width = Math.max(1, box.width),
    height = Math.max(1, box.height);
  const dpr = window.devicePixelRatio || 1;
  const pixelWidth = Math.round(width * dpr),
    pixelHeight = Math.round(height * dpr);
  if (canvas.width !== pixelWidth || canvas.height !== pixelHeight) {
    canvas.width = pixelWidth;
    canvas.height = pixelHeight;
  }
  const ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, width, height);
  ctx.font = '12px "Segoe UI", Arial, sans-serif';
  ctx.lineCap = 'round';
  return { ctx, width, height };
}

export function line(ctx, x1, y1, x2, y2, color, thickness = 1) {
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.lineTo(x2, y2);
  ctx.strokeStyle = color;
  ctx.lineWidth = thickness;
  ctx.stroke();
}
