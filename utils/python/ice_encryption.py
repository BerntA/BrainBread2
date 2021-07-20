import os
import numpy as np

BLOCK_SIZE = 8

ICE_SMOD = np.array([
    [333, 313, 505, 369],
    [379, 375, 319, 391],
    [361, 445, 451, 397],
    [397, 425, 395, 505]
], dtype=np.uint64)

ICE_SXOR = np.array([
    [0x83, 0x85, 0x9b, 0xcd],
    [0xcc, 0xa7, 0xad, 0x41],
    [0x4b, 0x2e, 0xd4, 0x33],
    [0xea, 0xcb, 0x2e, 0x04]
], dtype=np.uint64)

ICE_PBOX = np.array([
    0x00000001, 0x00000080, 0x00000400, 0x00002000,
    0x00080000, 0x00200000, 0x01000000, 0x40000000,
    0x00000008, 0x00000020, 0x00000100, 0x00004000,
    0x00010000, 0x00800000, 0x04000000, 0x20000000,
    0x00000004, 0x00000010, 0x00000200, 0x00008000,
    0x00020000, 0x00400000, 0x08000000, 0x10000000,
    0x00000002, 0x00000040, 0x00000800, 0x00001000,
    0x00040000, 0x00100000, 0x02000000, 0x80000000  
], dtype=np.uint64)

ICE_KEYROT = np.array([
    0, 1, 2, 3, 2, 1, 3, 0,
    1, 3, 2, 0, 3, 1, 0, 2
], dtype=np.uint64)

# Ensure that these never change! scream loudly, if ever.
ICE_SMOD.flags.writeable = False
ICE_SXOR.flags.writeable = False
ICE_PBOX.flags.writeable = False
ICE_KEYROT.flags.writeable = False

def gf_mult(a, b, m):
    res = np.uint64(0)
    while b:
        if (b & np.uint64(1)):
            res = res ^ a
        
        a = a << np.uint64(1)
        b = b >> np.uint64(1)
        
        if a >= 256:
            a = a ^ m
            
    return res

def gf_exp7(b, m):
    if b == np.uint64(0):
        return np.uint64(0)
    
    x = gf_mult(b, b, m)
    x = gf_mult(b, x, m)
    x = gf_mult(x, x, m)
    
    return (gf_mult(b, x, m))

def ice_perm32(x):
    idx = 0
    res = np.uint64(0)
    x = np.uint64(x)
    while x:
        if (x & np.uint64(1)):
            res = res | ICE_PBOX[idx]
        idx += 1
        x = x >> np.uint64(1)
    return res

def ice_f(p, sk):
    tl = ((p >> np.uint64(16)) & np.uint64(0x3ff)) | (((p >> np.uint64(14)) | (p << np.uint64(18))) & np.uint64(0xffc00))
    tr = (p & np.uint64(0x3ff)) | ((p << np.uint64(2)) & np.uint64(0xffc00))
    
    al = sk[2] & (tl ^ tr)
    ar = al ^ tr
    al ^= tl
    al ^= sk[0]
    ar ^= sk[1]

    return (
        ICE_SBOX[0][al >> np.uint64(10)] | 
        ICE_SBOX[1][al & np.uint64(0x3ff)]| 
        ICE_SBOX[2][ar >> np.uint64(10)] | 
        ICE_SBOX[3][ar & np.uint64(0x3ff)]
    )
		
ICE_SBOX = np.zeros((4,1024), dtype=np.uint64)
for z in range(1024):
    i = np.uint64(z)
    col = ((i >> np.uint64(1)) & np.uint64(0xff))
    row = ((i & np.uint64(0x1)) | ((i & np.uint64(0x200)) >> np.uint64(8)))
    
    x = (gf_exp7(col ^ ICE_SXOR[0][row], ICE_SMOD[0][row]) << np.uint64(24))
    ICE_SBOX[0][i] = ice_perm32(x)
    
    x = (gf_exp7(col ^ ICE_SXOR[1][row], ICE_SMOD[1][row]) << np.uint64(16))
    ICE_SBOX[1][i] = ice_perm32(x)

    x = (gf_exp7(col ^ ICE_SXOR[2][row], ICE_SMOD[2][row]) << np.uint64(8))
    ICE_SBOX[2][i] = ice_perm32(x)

    x = gf_exp7(col ^ ICE_SXOR[3][row], ICE_SMOD[3][row])
    ICE_SBOX[3][i] = ice_perm32(x)

ICE_SBOX.flags.writeable = False

class IceKey():
    
    """
    ICE ENCRYPTION, PORTED FROM VICE, http://www.darkside.com.au/ice/ by Bernt A. Eide
    USED FOR ENCRYPTING GAME DATA IN THE SOURCE ENGINE.
    FINALLY YOU CAN ENCRYPT/DECRYPT LOTS OF FILES WITHOUT RELYING ON STEAM(SOURCE), ->
    PLUS WITHOUT CONSTANT CRASHES AND MEMORY LEAKS!
    """
    
    def __init__(self, key, n=0):        
        if n < 1:
            self.size = 1
            self.rounds = 8
        else:
            self.size = n
            self.rounds = n * 16
        
        self.key = None
        self.keysched = np.zeros((self.rounds, 3), dtype=np.uint64)
        self.assign(key)
        
    def keysize(self):
        return np.uint64(self.size * 8)
    
    def blocksize(self):
        return np.uint64(8)
    
    def assign(self, key):
        """
        Build and set key
        """
        self.key = np.array([ord(c) for c in key], dtype=np.uint64)
        if self.rounds == 8:
            KB = np.array([0,0,0,0], dtype=np.uint64)
            for i in range(4):
                KB[3-i] = (self.key[i*2] << np.uint64(8)) | self.key[i*2+1]
            self.build(KB, 0, ICE_KEYROT)
        else: # Haven't tested this part!!
            for i in range(self.size):
                KB = np.array([0,0,0,0], dtype=np.uint64)
                for j in range(4):
                    KB[3-j] = (self.key[i*8 + j*2] << np.uint64(8)) | self.key[i*8 + j*2 + 1]
                self.build(KB, i*8, ICE_KEYROT)
                self.build(KB, self.rounds - 8 - i*8, ICE_KEYROT[8:])
        
    def build(self, KB, n, keyrot):
        for i in range(8):
            KR = np.uint64(keyrot[i])                        
            for j in range(3): # reset keysched for row n+i
                self.keysched[n+i,j] = np.uint64(0)
                
            isk = self.keysched[n+i,:]            
            for j in range(15):
                for k in range(4):
                    bit = (KB[(KR + np.uint64(k)) & np.uint64(3)] & np.uint64(1))
                    isk[j % 3] = (isk[j % 3] << np.uint64(1)) | bit
                    KB[(KR + np.uint64(k)) & np.uint64(3)] = (KB[(KR + np.uint64(k)) & np.uint64(3)] >> np.uint64(1)) | ((bit ^ np.uint64(1)) << np.uint64(15))
                        
    def encrypt(self, data):
        l = (data[0] << np.uint64(24)) | (data[1] << np.uint64(16)) | (data[2] << np.uint64(8)) | data[3]
        r = (data[4] << np.uint64(24)) | (data[5] << np.uint64(16)) | (data[6] << np.uint64(8)) | data[7]
        
        for i in range(0, self.rounds, 2):
            l = l ^ ice_f(r, self.keysched[i,:])
            r = r ^ ice_f(l, self.keysched[i+1,:])
            
        for i in range(4):
            data[3 - i] = r & np.uint64(0xFF)
            data[7 - i] = l & np.uint64(0xFF)
            
            r = r >> np.uint64(8)
            l = l >> np.uint64(8)
        
        return data
    
    def decrypt(self, data):
        l = (data[0] << np.uint64(24)) | (data[1] << np.uint64(16)) | (data[2] << np.uint64(8)) | data[3]
        r = (data[4] << np.uint64(24)) | (data[5] << np.uint64(16)) | (data[6] << np.uint64(8)) | data[7]
        
        for i in range(self.rounds-1, 0, -2):
            l = l ^ ice_f(r, self.keysched[i,:])
            r = r ^ ice_f(l, self.keysched[i-1,:])
            
        for i in range(4):
            data[3 - i] = r & np.uint64(0xFF)
            data[7 - i] = l & np.uint64(0xFF)
            
            r = r >> np.uint64(8)
            l = l >> np.uint64(8)
        
        return data
			
def readAllBytes(file):
    try:
        with open(file, 'rb') as f:
            return np.array([v for v in f.read()], dtype=np.uint64)
    except:
        return np.zeros(1, dtype=np.uint64)

def processFile(file, key, decrypt=False):
    bytes_read = readAllBytes(file)            
    size = bytes_read.shape[0]
    bytesLeft = size
    pos = 0    
    obj = IceKey(key)
    output = np.zeros(size, dtype=np.uint8)
    
    while bytesLeft >= BLOCK_SIZE:
        output[pos:(pos+BLOCK_SIZE)] = obj.decrypt(bytes_read[pos:(pos+BLOCK_SIZE)].copy()) if decrypt else obj.encrypt(bytes_read[pos:(pos+BLOCK_SIZE)].copy())
        pos += BLOCK_SIZE
        bytesLeft -= BLOCK_SIZE
    
    if bytesLeft > 0: # Assign remaining unencrypted chunk.
        output[(size - bytesLeft):] = bytes_read[(size - bytesLeft):]
    
    return output.tobytes()

def convertFiles(target_folder, output_folder, target_ext, output_ext, key, decrypt=False):
    for path, _, files in os.walk(target_folder, topdown=True):
        for f in [x.lower() for x in files if x.lower().endswith(target_ext)]:
            result = processFile('{}/{}'.format(path, f), key, decrypt)
            with open('{}/{}'.format(output_folder, f.replace(target_ext, output_ext)), 'wb') as out:
                out.write(result)
        break