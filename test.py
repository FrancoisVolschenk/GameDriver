import pygame
import os

file = "/dev/devDriver"

pygame.init()
screen = pygame.display.set_mode((800, 600))

player = pygame.Rect((0, 0, 50, 50))
current_colour = (255, 0, 0)

LB_CHECK = 2000 # 500
UB_CHECK = 2400 # 600

run = True

with open(file, "r") as fp:
	while run:
		try:
			state = fp.read().replace("_", "").split("-")
			if len(state) != 6:
				continue
				
			# print(state)
			xMove = int(state[0]) # //4
			yMove = int(state[1]) # //4
			b1 = int(state[2])
			b2 = int(state[3])
			b3 = int(state[4])
			b4 = int(state[5])

			screen.fill((0, 0, 0))

			pygame.draw.rect(screen, current_colour, player)
			if xMove < LB_CHECK:
				if player.x > 0:
					player.move_ip(-1, 0) # (-(xMove / LB_CHECK), 0) # -1
			elif xMove > UB_CHECK:
				if player.x <= (800 - player.width):
					player.move_ip(1, 0) # (((xMove - UB_CHECK)/ UB_CHECK), 0) # 1
			if yMove < LB_CHECK:
				if player.y > 0:
					player.move_ip(0, -1) # (0, -(yMove / LB_CHECK)) # -1
			elif yMove > UB_CHECK:
				if player.y <= (600 - player.height):
					player.move_ip(0, 1) # (0, ((yMove - UB_CHECK)/UB_CHECK)) # 1

			if b1 == 0:
				current_colour = (0, 0, 255)
			if b2 == 0:
				current_colour = (0, 255, 0)
			if b3 == 0:
				current_colour = (0, 0, 0)
			if b4 == 0:
				current_colour = (255, 0, 0)
				

			for event in pygame.event.get():
				if event.type == pygame.QUIT:
					run = False
				if event.type == pygame.KEYDOWN:
					if event.key == pygame.K_s:
						os.system("./invoke_ioctl")
			pygame.display.update()
		except:
			print("filtering noie from serial line")

	pygame.quit()
	fp.close()
