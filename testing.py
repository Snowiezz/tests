import pygame
import sys
import math
import random

# Initialize Pygame
pygame.init()

# Game constants
SCREEN_WIDTH = 800
SCREEN_HEIGHT = 600
FPS = 60

# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
RED = (255, 0, 0)
GREEN = (0, 255, 0)
BLUE = (0, 100, 255)
YELLOW = (255, 255, 0)

# Player constants
PLAYER_SIZE = 40
PLAYER_SPEED = 5

# Bullet constants
BULLET_SIZE = 8
BULLET_SPEED = 10

# Enemy constants
ENEMY_SIZE = 35
ENEMY_SPEED = 2
ENEMY_SPAWN_RATE = 60  # frames between spawns

class Player:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.size = PLAYER_SIZE
        self.speed = PLAYER_SPEED
        self.health = 100
        
    def move(self, keys):
        # WASD movement
        if keys[pygame.K_w] or keys[pygame.K_UP]:
            self.y -= self.speed
        if keys[pygame.K_s] or keys[pygame.K_DOWN]:
            self.y += self.speed
        if keys[pygame.K_a] or keys[pygame.K_LEFT]:
            self.x -= self.speed
        if keys[pygame.K_d] or keys[pygame.K_RIGHT]:
            self.x += self.speed
            
        # Keep player on screen
        self.x = max(self.size // 2, min(SCREEN_WIDTH - self.size // 2, self.x))
        self.y = max(self.size // 2, min(SCREEN_HEIGHT - self.size // 2, self.y))
    
    def draw(self, screen):
        pygame.draw.circle(screen, GREEN, (int(self.x), int(self.y)), self.size // 2)
        # Draw health bar
        bar_width = 50
        bar_height = 5
        health_ratio = self.health / 100
        pygame.draw.rect(screen, RED, (self.x - bar_width // 2, self.y - self.size // 2 - 10, bar_width, bar_height))
        pygame.draw.rect(screen, GREEN, (self.x - bar_width // 2, self.y - self.size // 2 - 10, bar_width * health_ratio, bar_height))

class Bullet:
    def __init__(self, x, y, target_x, target_y):
        self.x = x
        self.y = y
        self.size = BULLET_SIZE
        self.speed = BULLET_SPEED
        
        # Calculate direction
        dx = target_x - x
        dy = target_y - y
        distance = math.sqrt(dx**2 + dy**2)
        
        if distance > 0:
            self.dx = (dx / distance) * self.speed
            self.dy = (dy / distance) * self.speed
        else:
            self.dx = 0
            self.dy = 0
    
    def update(self):
        self.x += self.dx
        self.y += self.dy
    
    def is_off_screen(self):
        return (self.x < 0 or self.x > SCREEN_WIDTH or 
                self.y < 0 or self.y > SCREEN_HEIGHT)
    
    def draw(self, screen):
        pygame.draw.circle(screen, YELLOW, (int(self.x), int(self.y)), self.size // 2)

class Enemy:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        self.size = ENEMY_SIZE
        self.speed = ENEMY_SPEED
        self.health = 2
    
    def update(self, player):
        # Move towards player
        dx = player.x - self.x
        dy = player.y - self.y
        distance = math.sqrt(dx**2 + dy**2)
        
        if distance > 0:
            self.x += (dx / distance) * self.speed
            self.y += (dy / distance) * self.speed
    
    def draw(self, screen):
        pygame.draw.circle(screen, RED, (int(self.x), int(self.y)), self.size // 2)
    
    def collides_with(self, other_x, other_y, other_size):
        distance = math.sqrt((self.x - other_x)**2 + (self.y - other_y)**2)
        return distance < (self.size + other_size) / 2

class Game:
    def __init__(self):
        self.screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
        pygame.display.set_caption("Top-Down Shooter")
        self.clock = pygame.time.Clock()
        self.running = True
        self.game_over = False
        
        # Game objects
        self.player = Player(SCREEN_WIDTH // 2, SCREEN_HEIGHT // 2)
        self.bullets = []
        self.enemies = []
        
        # Game stats
        self.score = 0
        self.enemy_spawn_timer = 0
        
        # Font
        self.font = pygame.font.Font(None, 36)
        self.small_font = pygame.font.Font(None, 24)
    
    def spawn_enemy(self):
        # Spawn enemy at random edge of screen
        edge = random.choice(['top', 'bottom', 'left', 'right'])
        
        if edge == 'top':
            x = random.randint(0, SCREEN_WIDTH)
            y = -ENEMY_SIZE
        elif edge == 'bottom':
            x = random.randint(0, SCREEN_WIDTH)
            y = SCREEN_HEIGHT + ENEMY_SIZE
        elif edge == 'left':
            x = -ENEMY_SIZE
            y = random.randint(0, SCREEN_HEIGHT)
        else:  # right
            x = SCREEN_WIDTH + ENEMY_SIZE
            y = random.randint(0, SCREEN_HEIGHT)
        
        self.enemies.append(Enemy(x, y))
    
    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            
            # Shoot on mouse click
            if event.type == pygame.MOUSEBUTTONDOWN and not self.game_over:
                if event.button == 1:  # Left click
                    mouse_x, mouse_y = pygame.mouse.get_pos()
                    self.bullets.append(Bullet(self.player.x, self.player.y, mouse_x, mouse_y))
            
            # Restart on R key
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_r and self.game_over:
                    self.__init__()
    
    def update(self):
        if self.game_over:
            return
        
        # Player movement
        keys = pygame.key.get_pressed()
        self.player.move(keys)
        
        # Update bullets
        for bullet in self.bullets[:]:
            bullet.update()
            if bullet.is_off_screen():
                self.bullets.remove(bullet)
        
        # Update enemies
        for enemy in self.enemies[:]:
            enemy.update(self.player)
            
            # Check collision with player
            if enemy.collides_with(self.player.x, self.player.y, self.player.size):
                self.player.health -= 1
                self.enemies.remove(enemy)
                
                if self.player.health <= 0:
                    self.game_over = True
        
        # Check bullet-enemy collisions
        for bullet in self.bullets[:]:
            for enemy in self.enemies[:]:
                if enemy.collides_with(bullet.x, bullet.y, bullet.size):
                    enemy.health -= 1
                    if bullet in self.bullets:
                        self.bullets.remove(bullet)
                    
                    if enemy.health <= 0:
                        if enemy in self.enemies:
                            self.enemies.remove(enemy)
                        self.score += 10
                    break
        
        # Spawn enemies
        self.enemy_spawn_timer += 1
        if self.enemy_spawn_timer >= ENEMY_SPAWN_RATE:
            self.spawn_enemy()
            self.enemy_spawn_timer = 0
    
    def draw(self):
        self.screen.fill(BLACK)
        
        # Draw game objects
        self.player.draw(self.screen)
        
        for bullet in self.bullets:
            bullet.draw(self.screen)
        
        for enemy in self.enemies:
            enemy.draw(self.screen)
        
        # Draw UI
        score_text = self.small_font.render(f"Score: {self.score}", True, WHITE)
        self.screen.blit(score_text, (10, 10))
        
        enemy_text = self.small_font.render(f"Enemies: {len(self.enemies)}", True, WHITE)
        self.screen.blit(enemy_text, (10, 40))
        
        # Draw instructions
        if not self.game_over:
            inst_text = self.small_font.render("WASD/Arrows: Move | Left Click: Shoot", True, WHITE)
            self.screen.blit(inst_text, (SCREEN_WIDTH // 2 - 200, SCREEN_HEIGHT - 30))
        
        # Draw game over screen
        if self.game_over:
            game_over_text = self.font.render("GAME OVER!", True, RED)
            self.screen.blit(game_over_text, (SCREEN_WIDTH // 2 - 100, SCREEN_HEIGHT // 2 - 50))
            
            final_score_text = self.font.render(f"Final Score: {self.score}", True, WHITE)
            self.screen.blit(final_score_text, (SCREEN_WIDTH // 2 - 120, SCREEN_HEIGHT // 2))
            
            restart_text = self.small_font.render("Press R to Restart", True, WHITE)
            self.screen.blit(restart_text, (SCREEN_WIDTH // 2 - 100, SCREEN_HEIGHT // 2 + 50))
        
        pygame.display.flip()
    
    def run(self):
        while self.running:
            self.handle_events()
            self.update()
            self.draw()
            self.clock.tick(FPS)
        
        pygame.quit()
        sys.exit()

if __name__ == "__main__":
    game = Game()
    game.run()
