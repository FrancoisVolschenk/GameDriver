import pygame

file = "/dev/devDriver"

pygame.init()
screen = pygame.display.set_mode((800, 600))

player = pygame.Rect((0, 0, 50, 50))
current_colour = (255, 0, 0)

run = True

with open(file, "r") as fp:
	while run:
		state = fp.read().replace("_", "").split("-")
		if len(state) != 6:
			continue
		xMove = int(state[0])//4
		yMove = int(state[1])//4
		b1 = int(state[2])
		b2 = int(state[3])
		b3 = int(state[4])
		b4 = int(state[5])

		screen.fill((0, 0, 0))

		pygame.draw.rect(screen, current_colour, player)
		if xMove < 500:
			if player.x > 0:
				player.move_ip(-1, 0)
		elif xMove > 600:
			if player.x <= (800 - player.width):
				player.move_ip(1, 0)
		if yMove < 500:
			if player.y > 0:
				player.move_ip(0, -1)
		elif yMove > 600:
			if player.y <= (600 - player.height):
				player.move_ip(0, 1)

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
		pygame.display.update()

	pygame.quit()
