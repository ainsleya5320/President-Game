from pathlib import Path
import base64
P=Path(__file__).parent
dest=Path('C:/Users/ainsl/.codex/visualizations/2026/09/23/01a0cbc0-bc91-7121-a59e-996faa5dad8e/oval-office-3d.html')
markup=(P/'viewer-fragment.template.html').read_text(encoding='utf-8').replace('__MODEL_BASE64__',base64.b64encode((P/'Oval_Office_Preview.glb.gz').read_bytes()).decode()).replace('__PALETTE__',(P/'material_palette.json').read_text())
dest.write_text(markup,encoding='utf-8')
assert dest.stat().st_size<1_000_000
print(dest, dest.stat().st_size)
