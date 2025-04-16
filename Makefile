NAME			:= ircserv
BONUS_NAME		:= ircserv_bonus

# Répertoires
SRCS_DIR		:= src
OBJS_DIR		:= .obj
INCS_DIR		:= includes

# Compilation
CXX				:= c++
FLAGXX			:= -std=c++98 -Wall -Wextra -Werror -g
IFLAGS			:= -I $(INCS_DIR)

# Sources
SRCS			:=	main.cpp \
					Server.cpp \
					Channel.cpp \
					User.cpp \
					Command.cpp \
					commands/handleInvite.cpp \
					commands/handleJoin.cpp \
					commands/handleKick.cpp \
					commands/handleMode.cpp \
					commands/handleNick.cpp \
					commands/handlePart.cpp \
					commands/handlePass.cpp \
					commands/handlePrivmsg.cpp \
					commands/handleQuit.cpp \
					commands/handleTopic.cpp \
					commands/handleUser.cpp \
					commands/sendWelcomeMessages.cpp \

# Sources pour le bonus
BONUS_SRCS		:=	main_bonus.cpp \
					Bot_bonus.cpp \
					Server.cpp \
					Channel.cpp \
					User.cpp \
					Command.cpp \
					commands/handleInvite.cpp \
					commands/handleJoin.cpp \
					commands/handleKick.cpp \
					commands/handleMode.cpp \
					commands/handleNick.cpp \
					commands/handlePart.cpp \
					commands/handlePass.cpp \
					commands/handlePrivmsg.cpp \
					commands/handleQuit.cpp \
					commands/handleTopic.cpp \
					commands/handleUser.cpp \
					commands/sendWelcomeMessages.cpp \

# Ajout des préfixes et génération des objets
SRCS			:= $(addprefix $(SRCS_DIR)/, $(SRCS))
OBJS			:= $(SRCS:%.cpp=$(OBJS_DIR)/%.o)

BONUS_SRCS		:= $(addprefix $(SRCS_DIR)/, $(BONUS_SRCS))
BONUS_OBJS		:= $(BONUS_SRCS:%.cpp=$(OBJS_DIR)/%.o)

# Commandes
RM				:= rm -rf
DIR_UP			= mkdir -p $(@D)

# Couleurs et emojis
RED				:= \033[1;31m
GREEN			:= \033[1;32m
YELLOW			:= \033[1;33m
BLUE			:= \033[1;34m
RESET			:= \033[0m

SUCXXESS_EMOJI	:= ✅
CLEAN_EMOJI		:= 🗑️
BUILD_EMOJI		:= 🛠️
BONUS_EMOJI		:= 🎮

################################################################################

all: $(OBJS_DIR) $(NAME)

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)/$(SRCS_DIR)/commands

$(NAME): $(OBJS)
	@echo "$(YELLOW)$(BUILD_EMOJI)  Compilation in progress...$(RESET)"
	@$(CXX) $(FLAGXX) $(OBJS) -o $@
	@echo "$(GREEN)$(SUCXXESS_EMOJI) Compilation complete !$(RESET)"

bonus: $(OBJS_DIR) $(BONUS_NAME)

$(BONUS_NAME): $(BONUS_OBJS)
	@echo "$(BLUE)$(BONUS_EMOJI) Compilation bonus in progress...$(RESET)"
	@$(CXX) $(FLAGXX) $(BONUS_OBJS) -o $@
	@echo "$(GREEN)$(SUCXXESS_EMOJI) Compilation bonus complete !$(RESET)"

$(OBJS_DIR)/%.o: %.cpp
	@$(DIR_UP)
	@echo "$(YELLOW)$(BUILD_EMOJI)  Compilation of $<...$(RESET)"
	@$(CXX) $(FLAGXX) $(IFLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJS_DIR)
	@echo "$(RED)$(CLEAN_EMOJI)  Objects deleted!$(RESET)"

fclean: clean
	@$(RM) $(NAME) $(BONUS_NAME)
	@echo "$(RED)$(CLEAN_EMOJI)  Executable deleted!$(RESET)"

re: fclean all

.PHONY: all clean fclean re bonus
