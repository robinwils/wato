FROM golang:1.25-alpine AS builder

WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 go build -o watohttpd .

FROM alpine:3.21

RUN apk add --no-cache ca-certificates bash

WORKDIR /app
COPY --from=builder /app/watohttpd .

COPY entrypoint.sh .
RUN chmod +x entrypoint.sh

EXPOSE 8090

ENTRYPOINT ["./entrypoint.sh"]
