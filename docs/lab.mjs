import { surface, line } from './canvas.mjs';

const byId = (id) => document.getElementById(id);
const plot = byId('plot'),
  history = byId('history'),
  frame = byId('frame');
const profile = byId('profile'),
  method = byId('method'),
  ratio = byId('ratio');
const notes = {
  'forward-euler':
    'Uses the current temperature gradient. Its stable range on this grid is r ≤ 0.5.',
  'backward-euler':
    'Solves a tridiagonal system at the new time. Large stable steps can still lose accuracy.',
  'crank-nicolson':
    'Averages diffusive fluxes at the old and new times. The update is second order in time.',
};
let dataset, run, playing;
const text = (id, value) => {
  byId(id).textContent = value;
};

function stop() {
  clearInterval(playing);
  playing = undefined;
  text('play', 'Play');
}
function current() {
  return run.frames[Number(frame.value)];
}
function reference(i, time) {
  const wave = Math.PI / dataset.length;
  return (
    5.5 +
    4.5 *
      Math.cos((Math.PI * (i + 0.5)) / dataset.cells) *
      Math.exp(-dataset.diffusivity * wave * wave * time)
  );
}
function graph(values, initial, time) {
  const { ctx, width, height } = surface(plot);
  const left = 39,
    right = width - 12,
    top = 18,
    bottom = height - 34;
  const x = (pos) => left + pos * (right - left);
  const y = (val) => bottom - (val / 11) * (bottom - top);
  ctx.fillStyle = '#c2d8d8';
  ctx.textAlign = 'right';
  for (const v of [0, 2, 4, 6, 8, 10]) {
    line(ctx, left, y(v), right, y(v), '#345159');
    ctx.fillText(String(v), left - 8, y(v) + 4);
  }
  ctx.textAlign = 'center';
  for (const v of [0, 0.25, 0.5, 0.75, 1]) {
    line(ctx, x(v), top, x(v), bottom, '#24424a');
    ctx.fillText(String(v * dataset.length), x(v), bottom + 17);
  }
  ctx.fillText('Position, x', (left + right) / 2, height - 1);
  ctx.textAlign = 'left';
  ctx.fillText('T', 4, top + 2);
  function curve(series, color, pattern, weight) {
    ctx.beginPath();
    ctx.setLineDash(pattern);
    series.forEach((v, i) => {
      const px = x((i + 0.5) / series.length);
      if (i) ctx.lineTo(px, y(v));
      else ctx.moveTo(px, y(v));
    });
    ctx.lineWidth = weight;
    ctx.strokeStyle = color;
    ctx.stroke();
    ctx.setLineDash([]);
  }
  curve(initial, '#8bb4be', [5, 5], 1.6);
  curve(values, '#ffb776', [], 3.2);
  if (run.profile === 'cosine')
    curve(
      values.map((v, i) => reference(i, time)),
      '#d7eeb0',
      [1, 7],
      2.1,
    );
}
function heatMap() {
  const { ctx, width, height } = surface(history);
  const left = 52,
    right = width - 12,
    top = 12,
    bottom = height - 12;
  const rows = run.frames.length,
    dx = (right - left) / dataset.cells,
    dy = (bottom - top) / rows;
  const stops = [
    [43, 102, 152],
    [91, 160, 172],
    [231, 227, 173],
    [240, 155, 77],
    [192, 89, 47],
  ];
  for (let row = 0; row < rows; row++) {
    run.frames[row].forEach((v, col) => {
      const u = Math.max(0, Math.min(4, ((v - 1) * 4) / 9));
      const i = Math.min(3, Math.floor(u));
      const rgb = stops[i].map((c, k) => Math.round(c + (stops[i + 1][k] - c) * (u - i)));
      ctx.fillStyle = 'rgb(' + rgb.join(',') + ')';
      ctx.fillRect(left + col * dx, top + row * dy, dx + 0.55, dy + 0.55);
    });
  }
  const cursor = top + (Number(frame.value) + 0.5) * dy;
  line(ctx, left, cursor, right, cursor, '#122a31', 3);
  line(ctx, left, cursor, right, cursor, '#fff', 1.4);
  const probeX = left + (Number(byId('probe').value) + 0.5) * dx;
  ctx.setLineDash([3, 4]);
  line(ctx, probeX, top, probeX, bottom, '#152d38', 2);
  ctx.setLineDash([]);
  ctx.fillStyle = '#d5e6e4';
  ctx.textAlign = 'right';
  ctx.fillText('t = 0', left - 6, top + 8);
  ctx.fillText((dataset.frameDt * (rows - 1)).toFixed(2), left - 6, bottom);
}
function render() {
  if (!dataset) return;
  if (!run) {
    const { ctx, width, height } = surface(plot);
    ctx.fillStyle = '#e4efec';
    ctx.textAlign = 'center';
    ctx.fillText('Choose r ≤ 0.5 or an implicit integrator.', width / 2, height / 2);
    surface(history);
    return;
  }
  const values = current(),
    t = Number(frame.value) * dataset.frameDt;
  text(
    'probe-label',
    'Probe at x = ' +
      (((Number(byId('probe').value) + 0.5) * dataset.length) / dataset.cells).toFixed(3),
  );
  graph(values, run.frames[0], t);
  heatMap();
  text('time', t.toFixed(3));
  text('frame-label', Number(frame.value) + ' / ' + (run.frames.length - 1));
  text('mean', (values.reduce((a, b) => a + b, 0) / values.length).toFixed(3));
  const analytic = run.profile === 'cosine';
  text('metric-title', analytic ? 'Max |T − exact|' : 'Temperature span');
  text(
    'span',
    analytic
      ? Math.max(...values.map((v, i) => Math.abs(v - reference(i, t)))).toExponential(2)
      : (Math.max(...values) - Math.min(...values)).toFixed(3),
  );
  byId('exact-key').hidden = !analytic;
}
function select() {
  stop();
  if (!dataset) return;
  run = dataset.runs.find(
    (r) =>
      r.profile === profile.value && r.method === method.value && r.ratio === Number(ratio.value),
  );
  text('scheme-note', notes[method.value]);
  byId('notice').hidden = Boolean(run);
  byId('download').disabled = byId('play').disabled = frame.disabled = !run;
  byId('probe-download').disabled = byId('probe').disabled = !run;
  if (run) frame.max = run.frames.length - 1;
  else {
    text(
      'notice',
      'Forward Euler at r = 1 exceeds its stability limit. Select r ≤ 0.5 or an implicit integrator.',
    );
    for (const id of ['time', 'mean', 'span', 'frame-label']) text(id, '—');
    byId('exact-key').hidden = true;
  }
  render();
}
for (const field of [profile, method, ratio]) field.addEventListener('change', select);
frame.addEventListener('input', () => {
  stop();
  render();
});
byId('play').addEventListener('click', () => {
  if (playing) {
    stop();
    return;
  }
  if (Number(frame.value) === Number(frame.max)) frame.value = 0;
  text('play', 'Pause');
  render();
  playing = setInterval(() => {
    frame.value = Number(frame.value) + 1;
    render();
    if (Number(frame.value) >= Number(frame.max)) stop();
  }, 140);
});
function downloadCsv(rows, suffix) {
  const url = URL.createObjectURL(
    new Blob([['x,time,temperature', ...rows].join('\n') + '\n'], { type: 'text/csv' }),
  );
  const link = document.createElement('a');
  link.href = url;
  link.download = run.profile + '-' + run.method + '-' + suffix + '.csv';
  link.click();
  setTimeout(() => URL.revokeObjectURL(url), 1500);
}
byId('download').addEventListener('click', () => {
  if (!run) return;
  const time = Number(frame.value) * dataset.frameDt;
  downloadCsv(
    current().map((v, i) => [((i + 0.5) * dataset.length) / dataset.cells, time, v].join(',')),
    'frame-' + frame.value,
  );
});
byId('probe').addEventListener('input', render);
byId('probe-download').addEventListener('click', () => {
  if (!run) return;
  const cell = Number(byId('probe').value),
    x = ((cell + 0.5) * dataset.length) / dataset.cells;
  downloadCsv(
    run.frames.map((row, i) => [x, i * dataset.frameDt, row[cell]].join(',')),
    'probe-' + cell,
  );
});
new ResizeObserver(render).observe(plot);
try {
  const response = await fetch('data/profiles.json');
  if (!response.ok) throw new Error('HTTP ' + response.status);
  dataset = await response.json();
  select();
} catch (error) {
  byId('notice').hidden = false;
  text('notice', 'Could not load the C++ profiles. Reload this page or open the hosted demo.');
}
