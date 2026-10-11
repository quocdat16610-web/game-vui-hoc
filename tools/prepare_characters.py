#!/usr/bin/env python3
"""
Chuẩn bị model nhân vật KayKit Adventurers (CC0) cho game:
  - Giữ nguyên 1 file đầy đủ hoạt ảnh (animations.glb) để mọi nhân vật dùng chung
  - Các file nhân vật khác: xóa hoạt ảnh + dữ liệu thừa -> nhẹ hơn rất nhiều
  - Tạo file .attach: mesh nào (mũ, áo choàng, vũ khí) gắn vào xương nào,
    vì raylib 5.5 chưa tự gắn mesh cứng (không skin) vào xương.
Dùng: python3 tools/prepare_characters.py <thư_mục_gltf_KayKit> assets/models/characters
"""
import json, struct, sys, os

def read_glb(path):
    d = open(path, 'rb').read()
    jl = struct.unpack('<I', d[12:16])[0]
    j = json.loads(d[20:20 + jl])
    off = 20 + jl
    bl = struct.unpack('<I', d[off:off + 4])[0]
    b = d[off + 8: off + 8 + bl]
    return j, b

def write_glb(path, j, b):
    js = json.dumps(j, separators=(',', ':')).encode()
    js += b' ' * ((4 - len(js) % 4) % 4)
    b += b'\0' * ((4 - len(b) % 4) % 4)
    total = 12 + 8 + len(js) + 8 + len(b)
    with open(path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<II', len(js), 0x4E4F534A)); f.write(js)
        f.write(struct.pack('<II', len(b), 0x004E4942)); f.write(b)

def strip_animations(j, b):
    j.pop('animations', None)
    used_acc = set()
    for m in j['meshes']:
        for p in m['primitives']:
            used_acc.update(p['attributes'].values())
            if 'indices' in p: used_acc.add(p['indices'])
    for s in j.get('skins', []):
        if 'inverseBindMatrices' in s: used_acc.add(s['inverseBindMatrices'])
    used_bv = set(j['accessors'][a]['bufferView'] for a in used_acc if 'bufferView' in j['accessors'][a])
    for im in j.get('images', []):
        if 'bufferView' in im: used_bv.add(im['bufferView'])
    acc_map, bv_map = {}, {}
    new_acc = []
    for i, a in enumerate(j['accessors']):
        if i in used_acc: acc_map[i] = len(new_acc); new_acc.append(a)
    new_bv, nb = [], bytearray()
    for i, v in enumerate(j['bufferViews']):
        if i not in used_bv: continue
        bv_map[i] = len(new_bv)
        start = v.get('byteOffset', 0)
        data = b[start:start + v['byteLength']]
        while len(nb) % 4: nb.append(0)
        v = dict(v); v['byteOffset'] = len(nb)
        nb += data
        new_bv.append(v)
    for a in new_acc:
        if 'bufferView' in a: a['bufferView'] = bv_map[a['bufferView']]
    for m in j['meshes']:
        for p in m['primitives']:
            p['attributes'] = {k: acc_map[v] for k, v in p['attributes'].items()}
            if 'indices' in p: p['indices'] = acc_map[p['indices']]
    for s in j.get('skins', []):
        if 'inverseBindMatrices' in s: s['inverseBindMatrices'] = acc_map[s['inverseBindMatrices']]
    for im in j.get('images', []):
        if 'bufferView' in im: im['bufferView'] = bv_map[im['bufferView']]
    j['accessors'] = new_acc
    j['bufferViews'] = new_bv
    j['buffers'] = [{'byteLength': len(nb)}]
    return j, bytes(nb)

def attach_info(j):
    """Raylib nạp mesh theo THỨ TỰ NODE (mỗi primitive là 1 mesh). Với mỗi mesh ghi:
    chỉ số, tên node, 'skin' (đã gắn xương sẵn) hoặc chỉ số xương cha (mesh cứng)."""
    parent = {}
    for i, n in enumerate(j['nodes']):
        for c in n.get('children', []): parent[c] = i
    joints = j['skins'][0]['joints'] if j.get('skins') else []
    lines = []
    idx = 0
    for ni, n in enumerate(j['nodes']):
        if 'mesh' not in n: continue
        skinned = 'skin' in n
        bone = -1
        if not skinned:
            p = parent.get(ni)
            while p is not None and p not in joints: p = parent.get(p)
            if p is not None: bone = joints.index(p)
        for _ in j['meshes'][n['mesh']]['primitives']:
            lines.append(f"{idx} {n['name']} {'skin' if skinned else bone}")
            idx += 1
    return lines

src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
chars = {'Knight': 'knight', 'Barbarian': 'barbarian', 'Mage': 'mage', 'Rogue': 'rogue', 'Rogue_Hooded': 'rogue_hooded'}
for f, out in chars.items():
    j, b = read_glb(os.path.join(src, f + '.glb'))
    open(os.path.join(dst, out + '.attach'), 'w').write('\n'.join(attach_info(j)) + '\n')
    if f == 'Knight':
        jj, bb = read_glb(os.path.join(src, f + '.glb'))
        write_glb(os.path.join(dst, 'animations.glb'), jj, bb)   # đầy đủ hoạt ảnh
    j2, b2 = strip_animations(j, b)
    write_glb(os.path.join(dst, out + '.glb'), j2, b2)
    print(out, os.path.getsize(os.path.join(dst, out + '.glb')))
