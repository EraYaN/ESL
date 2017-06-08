c = {'Naive', 'Optimised', 'Optimised (fixp.)', 'NEON', 'DSP', 'DSP + Optimised', 'DSP + NEON'};
speedups = [1 1; 14.3 4.4; 0 0; 20 4.7; 0 0; 0 0; 0 0];
bar(speedups)
line(xlim, [1 1], 'LineStyle', '--', 'Color', [0.2, 0.2, 0.2]);
ax = gca;
ax.XTickLabelRotation = 45;
ax.XTickLabel = c;
ylabel('Speed-up [x]')

addpath('matlab2tikz')
matlab2tikz( '../docs/assignment2/comparison/resources/comparison.tikz', 'height', '\figureheight', 'width', '\figurewidth' );
