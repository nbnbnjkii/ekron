// Bake the user's original Canvas drawings into a native GLES atlas.
// npm ci --prefix tools/lobby-art && npm run export --prefix tools/lobby-art
import {createCanvas,GlobalFonts} from '@napi-rs/canvas';
import ts from 'typescript';
import {execFileSync} from 'node:child_process';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../../',import.meta.url));
GlobalFonts.registerFromPath(fileURLToPath(new URL('fonts/RussoOne-Regular.ttf',import.meta.url)),'Russo One');
GlobalFonts.registerFromPath(fileURLToPath(new URL('fonts/JetBrainsMono[wght].ttf',import.meta.url)),'JetBrains Mono');
let source=execFileSync('python3',['-c','import zipfile,sys;sys.stdout.buffer.write(zipfile.ZipFile(sys.argv[1]).read("src/game/textures.ts"))',root+'military-lobby-ui-design.zip'],{maxBuffer:4*1024*1024}).toString();
source=source.replace('import * as THREE from "three";',`const THREE={CanvasTexture:class {image:any;repeat={set(){}};constructor(c:any){this.image=c}},SRGBColorSpace:0,NoColorSpace:0,RepeatWrapping:0};`);
const js=ts.transpileModule(source,{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText;
const ex={};new Function('exports','document',js)(ex,{createElement(){return createCanvas(1,1)}});
const tiles=[
 ['banner',ex.makeBanner()],['map',ex.makeMap()],['cabinet',ex.makeCabinet()],
 ['tactical-sign',ex.makeSign(['ТАКТИЧЕСКАЯ КАРТА'],640,90,{fg:'#d6d6bc',bg:'#1b211f',border:'#8a9a6a',spacing:5,size:38})],
 ...['01 · КОМАНДНЫЙ ПУНКТ','02 · НАСТРОЙКИ','03 · ИНТРО','04 · ПОЛЕВОЕ РУКОВОДСТВО'].map((s,i)=>['monitor-'+i,ex.makeSign([s],1024,110,{fg:'#d8f3e0',bg:'#101a15',border:'#6dffa8',spacing:6,size:52,font:ex.FONT_DISPLAY})]),
 ...[['БОЕПРИПАСЫ','ВЗРЫВООПАСНО'],['ЗИП','ЗАПЧАСТИ'],['ПАЙКИ','ПРОДОВОЛЬСТВИЕ'],['МЕД','АПТЕЧКИ'],['СВЯЗЬ','РАДИО'],['ИНСТР','ИНВЕНТАРЬ']].map((s,i)=>['crate-'+i,ex.makeCrate(s[0],s[1],101+i)]),
 ['barrel',ex.makeBarrel(7)],['hazard',ex.makeHazard()]
];
const atlas=createCanvas(2048,2048),ctx=atlas.getContext('2d');
const manifest=[];
for(let i=0;i<tiles.length;i++){
 const [name,t]=tiles[i],x=(i%4)*512,y=Math.floor(i/4)*512;
 ctx.drawImage(t.image,x+2,y+2,508,508);
 manifest.push({tile:i,name});
}
const output=root+'sys/android/app/src/main/assets/lobby/';mkdirSync(output,{recursive:true});
writeFileSync(output+'artwork.png',atlas.toBuffer('image/png'));
writeFileSync(output+'artwork.json',JSON.stringify(manifest,null,2)+'\n');
console.log(`Exported ${tiles.length} original-design tiles to lobby/artwork.png`);
