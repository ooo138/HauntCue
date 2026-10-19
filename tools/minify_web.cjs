// Build-only JavaScript compression. GPIO/configuration names and DOM IDs stay
// intact; property mangling and unsafe transforms are deliberately disabled.
const fs = require('node:fs');
const { minify } = require('terser');
async function main() {
  const [input, output] = process.argv.slice(2);
  if (!input || !output) throw Error('Usage: node tools/minify_web.cjs input.html output.html');
  const html = fs.readFileSync(input, 'utf8');
  const script = html.match(/<script>([\s\S]*?)<\/script>/);
  if (!script) throw Error('Expected one inline application script.');
  const result = await minify(script[1], {
    ecma: 2022,
    compress: { passes: 2 },
    mangle: { toplevel: true },
    format: { comments: false, inline_script: true }
  });
  if (!result.code) throw Error('Interface minification produced no code.');
  const compact = html.replace(script[0], '<script>' + result.code + '</script>');
  if (output === '-') process.stdout.write(compact);
  else fs.writeFileSync(output, compact);
}
main().catch(error => { console.error(error); process.exitCode = 1; });
