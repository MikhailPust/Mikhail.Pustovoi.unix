import redis
import json
import time
import os
import signal

redis_client = redis.Redis(host='redis', port=6379, db=0)
worker_id = os.environ.get('HOSTNAME', 'unknown-worker')

keep_running = True

def handle_shutdown(signum, frame):
    global keep_running
    print(f"[{worker_id}] A stop signal has been received")
    keep_running = False

signal.signal(signal.SIGTERM, handle_shutdown)
signal.signal(signal.SIGINT, handle_shutdown)

print(f"Worker {worker_id} started. Graceful shutdown active.")

while keep_running:
    try:
        task_raw = redis_client.brpoplpush('tasks_queue', 'processing', timeout=5)

        if task_raw:
            task_data = json.loads(task_raw.decode('utf-8'))
            t_id = task_data['task_id']
            a = float(task_data['a'])
            b = float(task_data['b'])

            print(f"[{worker_id}] Calculating task {t_id}: {a} + {b}")
            time.sleep(1)
            result = a + b

            redis_client.setex(
                f'result:{t_id}',
                600, 
                json.dumps({'sum': result, 'worker': worker_id})
            )

            redis_client.lrem('processing', 0, task_raw)
            print(f"[{worker_id}] Task {t_id} done. Result: {result}")
        
    except redis.exceptions.ConnectionError:
        print("Waiting for Redis...")
        time.sleep(5)
    except Exception as e:
        print(f"Error: {e}")
        time.sleep(1)

print(f"[{worker_id}] The worker is stopped ")
