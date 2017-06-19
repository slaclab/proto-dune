
def PrintValue(x,y):            
    print( 'Word[%i]:\t0x%016X'%(x,y) )
    
# Header
PrintValue(0,0x0000000111000000)
byte = [0,1,2,3,4,5,6,7]
idx = 0

# Payload
for i in range(1024):
    for j in range(30):
        # Check for seed
        if (j==0):
            for k in range(8):
                cnt = (i + k)
                byte[k] = (cnt & 0xFF)
        # Create the pattern
        value = 0
        for k in range(8):
            cnt = (byte[k] + 8*j)
            value |= (cnt & 0xFF) << 8*k
        # Print to screen
        idx += 1
        if (i==0)    and (j==0): PrintValue(idx,value)
        if (i==0)    and (j==1): PrintValue(idx,value)
        if (i==0)    and (j==2): PrintValue(idx,value)
        if (i==0)    and (j==2): print ('............')
        if (i==0)    and (j==28): PrintValue(idx,value)
        if (i==0)    and (j==29): PrintValue(idx,value)
        if (i==1)    and (j==0): PrintValue(idx,value)
        if (i==1)    and (j==1): PrintValue(idx,value)
        if (i==1)    and (j==2): PrintValue(idx,value)        
        if (i==0)    and (j==2): print ('............')
        if (i==1023) and (j==28): PrintValue(idx,value)
        if (i==1023) and (j==29): PrintValue(idx,value)        

# Footer
PrintValue(idx+1,0x708b309e1103c010)        
        