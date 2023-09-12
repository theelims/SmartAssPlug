import { sveltekit } from '@sveltejs/kit/vite';
import type { UserConfig } from 'vite';
import Icons from 'unplugin-icons/vite';
import viteLittleFS from './vite-plugin-littlefs';

const config: UserConfig = {
	plugins: [
		sveltekit(),
		Icons({
			compiler: 'svelte'
		}),
		// Shorten file names for LittleFS 32 char limit
		viteLittleFS()
	],
	server: {
		proxy: {
			// Proxying REST: http://localhost:5173/rest/bar -> http://192.168.1.83/rest/bar
			'/rest': {
				target: 'http://192.168.1.83',
				//target: 'http://192.168.1.95',
				changeOrigin: true
			},
			// Proxying REST: http://localhost:5173/rest/bar -> http://192.168.1.83/rest/bar
			'/events': {
				target: 'http://192.168.1.83',
				//target: 'http://192.168.1.95',
				changeOrigin: true
			},
			// Proxying websockets ws://localhost:5173/ws -> ws://192.168.1.83/ws
			'/ws': {
				target: 'ws://192.168.1.83',
				//target: 'ws://192.168.1.95',
				changeOrigin: true,
				ws: true
			},
			'/rawdata': {
				target: 'ws://192.168.1.83',
				//target: 'ws://192.168.1.95',
				changeOrigin: true
			}
		}
	}
};

export default config;
