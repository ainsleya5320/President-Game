"""Bundle data/*.json into the FourYearsData script that simulation-core.js reads."""
from pathlib import Path
import json


def data_script(folder):
    data = {path.stem: json.loads(path.read_text(encoding='utf-8')) for path in sorted(Path(folder).glob('*.json'))}
    return 'const FourYearsData=' + json.dumps(data, separators=(',', ':'), ensure_ascii=False) + ';'
