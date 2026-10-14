"""Import the authored limo with the same validated mesh/material pipeline as staff."""
import unreal as u,runpy
from pathlib import Path
P=Path(u.Paths.project_dir())
runpy.run_path(str(P/'Scripts/import_presidential_staff.py'),init_globals={
 'SOURCE_ASSET_DIRECTORY':'SourceAssets/PresidentialLimo',
 'ASSET_BASE':'/Game/PresidentialLimo','REPORT_PREFIX':'LimoImport'})
