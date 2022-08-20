const fs = require('fs')
const files = fs.readdirSync(__dirname).filter(f => f.endsWith('.bin')).map(f => fs.readFileSync(f));
for (let i=0; i<0x200; ++i) {
    let slices = Buffer.concat(
        files.map(b => Uint8Array.prototype.slice.call(b, i * 0x10, (i + 1) * 0x10))
    );
    process.stdout.write(new Uint8Array(slices));
}
