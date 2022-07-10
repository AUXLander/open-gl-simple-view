import matplotlib.pyplot as plt

fig = plt.figure(figsize=(20,20))

ax = fig.add_subplot(111, projection='3d')

for x in range(0, 10):
    for y in  range(0, 10):
        for z in range(0, 10):
            ax.scatter(x,y,z); 

plt.show()