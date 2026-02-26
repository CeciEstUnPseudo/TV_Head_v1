# PlatformIO extra script to automatically move speech recognition models before upload
# Executes prior to flashing so the partition 'model' contains expected WakeNet/MultiNet binaries.
# Adjust paths if your sdkconfig filename changes.

import os
import stat
import subprocess
import sys  # Needed for sys.executable
import re
import shutil
from SCons.Script import DefaultEnvironment

def _guess_sdkconfig(project_dir):
    # Try common sdkconfig names present in the repo
    candidates = [
        'sdkconfig',
        'sdkconfig.esp32-s3-devkitc1-n16r8',
        'sdkconfig.esp32-s3-devkitc-1-n16r8v'
    ]
    for c in candidates:
        p = os.path.join(project_dir, c)
        if os.path.isfile(p):
            return p
    return None

def _safe_rmtree(path):
    if not os.path.exists(path):
        return
    def _onerror(func, p, exc):
        try:
            os.chmod(p, stat.S_IWRITE)
            func(p)
        except Exception as e:
            print(f'[movemodel] Impossible de supprimer {p}: {e}')
    import shutil
    shutil.rmtree(path, onerror=_onerror)

def _compute_model_offset(partitions_csv_path):
    """Derive offset for 'model' partition. If offset column empty, compute as factory_offset+factory_size."""
    if not os.path.isfile(partitions_csv_path):
        return None
    factory_offset = None
    factory_size = None
    model_offset = None
    with open(partitions_csv_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = [p.strip() for p in line.split(',')]
            if len(parts) < 5:
                continue
            name = parts[0]
            offset_field = parts[3]
            size_field = parts[4]
            if name == 'factory':
                # offset always present for factory
                try:
                    factory_offset = int(offset_field, 16)
                except Exception:
                    pass
                # size may be like 2M
                factory_size = _parse_size(size_field)
            if name == 'model':
                if offset_field:
                    try:
                        model_offset = int(offset_field, 16)
                    except Exception:
                        model_offset = None
                # If blank we'll compute later
    if model_offset is None and factory_offset is not None and factory_size is not None:
        model_offset = factory_offset + factory_size
    return model_offset

def _parse_size(size_str):
    # Accept formats like 6000K, 2M, 0x200000
    s = size_str.strip().upper()
    try:
        if s.startswith('0X'):
            return int(s, 16)
        m = re.match(r'([0-9]+)([K|M])', s)
        if m:
            val = int(m.group(1))
            unit = m.group(2)
            if unit == 'K':
                return val * 1024
            if unit == 'M':
                return val * 1024 * 1024
        return int(s)
    except Exception:
        return None

def before_upload(source, target, env):
    project_dir = env['PROJECT_DIR']
    # Expand SCons variable (env['BUILD_DIR'] may still contain $PROJECT_BUILD_DIR/$PIOENV)
    build_dir = env.subst('$BUILD_DIR')
    build_dir = os.path.abspath(build_dir)
    esp_sr_dir = os.path.join(project_dir, 'components', 'esp-sr')
    model_dir = os.path.join(esp_sr_dir, 'model')
    movemodel_py = os.path.join(model_dir, 'movemodel.py')
    partitions_csv = os.path.join(project_dir, 'partitions.csv')

    sdkconfig_path = _guess_sdkconfig(project_dir)
    if not sdkconfig_path:
        print('[movemodel] Aucune sdkconfig trouvée, saut du déplacement des modèles.')
        return
    if not os.path.isfile(movemodel_py):
        print('[movemodel] Script movemodel.py introuvable: %s' % movemodel_py)
        return

    print('[movemodel] Déplacement des modèles (WakeNet/MultiNet) -> partition model')
    try:
        # Nettoyer répertoire srmodels existant
        target_srmodels = os.path.join(build_dir, 'srmodels')
        _safe_rmtree(target_srmodels)
        subprocess.run([
            sys.executable, movemodel_py,
            '-d1', sdkconfig_path,
            '-d2', esp_sr_dir,
            '-d3', build_dir
        ], check=True)
        print('[movemodel] Succès.')
    except subprocess.CalledProcessError as e:
        print('[movemodel] Échec du script movemodel.py:', e)
        return

    # Corriger ancien emplacement mal expansé si présent (littéral variables)
    legacy_dir = os.path.join(project_dir, '$PROJECT_BUILD_DIR', '%PIOENV', 'srmodels')
    if os.path.isdir(legacy_dir):
        try:
            for fn in os.listdir(legacy_dir):
                src_path = os.path.join(legacy_dir, fn)
                dst_path = os.path.join(target_srmodels, fn)
                if os.path.isfile(src_path):
                    shutil.copy2(src_path, dst_path)
            print('[movemodel] Legacy srmodels directory contenu migré.')
        except Exception as e:
            print('[movemodel] Migration legacy échouée:', e)

    # Enregistrer offset pour post-flash
    model_offset = _compute_model_offset(partitions_csv)
    if model_offset is None:
        print('[movemodel] Impossible de déterminer offset partition model, flash manuel requis.')
    else:
        env['MODEL_PART_OFFSET'] = model_offset
        env['MODEL_BIN_PATH'] = os.path.join(target_srmodels, 'srmodels.bin')

def flash_model_partition(source, target, env):
    bin_path = env.get('MODEL_BIN_PATH')
    offset = env.get('MODEL_PART_OFFSET')
    upload_port = env.get('UPLOAD_PORT') or env.subst('$UPLOAD_PORT')
    if not bin_path or not os.path.isfile(bin_path):
        print('[movemodel] Skip flash modèle (srmodels.bin introuvable).')
        return
    if offset is None:
        print('[movemodel] Skip flash modèle (offset indéterminé).')
        return
    if not upload_port:
        print('[movemodel] Skip flash modèle (UPLOAD_PORT manquant).')
        return
    # Localiser esptool dans tool-esptoolpy package
    esptool_py = os.path.join(os.path.expanduser('~'), '.platformio', 'packages', 'tool-esptoolpy', 'esptool.py')
    if not os.path.isfile(esptool_py):
        print(f'[movemodel] esptool.py introuvable à {esptool_py}, abandon flash modèle.')
        return
    size = os.path.getsize(bin_path)
    if size < 128:
        print(f'[movemodel] srmodels.bin semble trop petit ({size} bytes), flash ignoré.')
        return
    print(f'[movemodel] Flash modèle -> offset 0x{offset:08X} taille {size} bytes ({bin_path})')
    try:
        subprocess.run([
            sys.executable, esptool_py,
            '--chip','esp32s3','--port', upload_port,
            'write_flash','0x%X' % offset, bin_path
        ], check=True)
        print('[movemodel] Flash modèle terminé.')
    except subprocess.CalledProcessError as e:
        print('[movemodel] Échec flash modèle:', e)

# Register hook
env = DefaultEnvironment()
env.AddPreAction('upload', before_upload)
env.AddPostAction('upload', flash_model_partition)
