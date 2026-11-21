import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';

const load = (path) => JSON.parse(readFileSync(path, 'utf8'));
const published = load(new URL('../docs/data/profiles.json', import.meta.url));
const rebuilt = load(process.argv[2]);
assert.equal(published.schema, 1);
for (const key of ['schema', 'cells', 'length', 'diffusivity', 'frameDt'])
  assert.equal(published[key], rebuilt[key]);
assert.equal(published.runs.length, 24);
assert.equal(rebuilt.runs.length, 24);
const keys = new Set();
let readings = 0;
for (let i = 0; i < rebuilt.runs.length; i++) {
  const actual = rebuilt.runs[i],
    expected = published.runs[i];
  const id = [actual.profile, actual.method, actual.ratio].join('/');
  assert.ok(!keys.has(id));
  keys.add(id);
  for (const key of ['profile', 'method', 'ratio', 'dt']) assert.equal(actual[key], expected[key]);
  assert.equal(actual.frames.length, 41);
  assert.equal(expected.frames.length, 41);
  for (let f = 0; f < 41; f++) {
    const a = actual.frames[f],
      e = expected.frames[f];
    assert.equal(a.length, 64);
    assert.equal(e.length, 64);
    for (let cell = 0; cell < 64; cell++) {
      assert.ok(Math.abs(a[cell] - e[cell]) < 2e-8, id + ' frame ' + f + ' cell ' + cell);
      readings++;
    }
  }
}
console.log(keys.size + ' C++ runs, ' + readings + ' published temperature samples checked.');
const cli = (...args) =>
  execFileSync(process.argv[3], args, { encoding: 'utf8' })
    .trim()
    .split(/\r?\n/)
    .slice(1)
    .map((line) => line.split(',').map(Number));
for (const [x, cell] of [
  ['0', 0],
  ['4', 12],
  ['20', 63],
]) {
  const probe = cli('--steps', '3', '--probe', x);
  const profile = cli('--steps', '3');
  assert.equal(probe.length, 4);
  assert.equal(probe[0][1], 0);
  assert.ok(probe[1][1] > 0);
  assert.deepEqual(probe.at(-1), profile[cell]);
}
console.log(
  'Probe timestamps and final values match the profile at both ends and an interior cell.',
);
