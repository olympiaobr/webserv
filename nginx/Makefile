# Makefile for Docker Nginx Container

# Variables
DOCKER_IMAGE_NAME = test-server-nginx
CONTAINER_NAME = some-nginx
HOST_PORT = 8085
CONTAINER_PORT = 80

# Build the Docker image
build:
	docker build -t $(DOCKER_IMAGE_NAME) .

# Run the container in detached mode
run:
	docker run --name $(CONTAINER_NAME) -d -p $(HOST_PORT):$(CONTAINER_PORT) $(DOCKER_IMAGE_NAME)

# Stop the container
stop:
	docker stop $(CONTAINER_NAME)

# Remove the container
remove:
	docker rm $(CONTAINER_NAME)

# Stop and remove the container
clean: stop remove

# Rebuild the image and restart the container
rebuild: clean build run

# Access the container in interactive mode
interactive:
	docker exec -it $(CONTAINER_NAME) /bin/sh

# View container logs
logs:
	docker logs $(CONTAINER_NAME)

# Follow container logs
logs-follow:
	docker logs -f $(CONTAINER_NAME)

# Show container status
status:
	docker ps -a | grep $(CONTAINER_NAME)

# Restart the container
restart:
	docker restart $(CONTAINER_NAME)

# Copy a file into the container
# Usage: make copy-to-container FILE=path/to/local/file DEST=/path/in/container
copy-to-container:
	docker cp $(FILE) $(CONTAINER_NAME):$(DEST)

# Copy a file from the container
# Usage: make copy-from-container SRC=/path/in/container DEST=path/to/local/destination
copy-from-container:
	docker cp $(CONTAINER_NAME):$(SRC) $(DEST)

# Run nginx configuration test inside the container
nginx-test:
	docker exec $(CONTAINER_NAME) nginx -t

# Reload nginx configuration without stopping the container
nginx-reload:
	docker exec $(CONTAINER_NAME) nginx -s reload

# Show help
help:
	@echo "Available targets:"
	@echo "  build              - Build the Docker image"
	@echo "  run                - Run the container in detached mode"
	@echo "  stop               - Stop the container"
	@echo "  remove             - Remove the container"
	@echo "  clean              - Stop and remove the container"
	@echo "  rebuild            - Rebuild the image and restart the container"
	@echo "  interactive        - Access the container in interactive mode"
	@echo "  logs               - View container logs"
	@echo "  logs-follow        - Follow container logs"
	@echo "  status             - Show container status"
	@echo "  restart            - Restart the container"
	@echo "  copy-to-container  - Copy a file into the container"
	@echo "  copy-from-container- Copy a file from the container"
	@echo "  nginx-test         - Run nginx configuration test inside the container"
	@echo "  nginx-reload       - Reload nginx configuration without stopping the container"

.PHONY: build run stop remove clean rebuild interactive logs logs-follow status restart copy-to-container copy-from-container nginx-test nginx-reload help