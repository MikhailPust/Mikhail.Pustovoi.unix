from flask import Flask, request, jsonify
import redis
import uuid
import json

app = Flask(__name__)

redis_client = redis.Redis(host='redis', port=6379, db=0)

@app.route('/add', methods=['POST'])
def add_task():
  
    try:
        data = request.get_json()
        a = data.get('a')
        b = data.get('b')

        if a is None or b is None:
            return jsonify({'error': 'Fields "a" and "b" are required'}), 400

        task_id = str(uuid.uuid4())
        task_data = {
            'task_id': task_id,
            'a': a,
            'b': b
        }

        redis_client.lpush('tasks_queue', json.dumps(task_data))

        return jsonify({'task_id': task_id, 'status': 'queued'}), 202

    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/result/<task_id>', methods=['GET'])
def get_result(task_id):
    result_raw = redis_client.get(f'result:{task_id}')
    
    if result_raw:
        return jsonify({
            'task_id': task_id, 
            'status': 'completed', 
            'result': json.loads(result_raw)
        })
    
    return jsonify({'task_id': task_id, 'status': 'processing'}), 202

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
